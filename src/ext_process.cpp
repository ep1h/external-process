#include "ext_process.hpp"
#include "core/ep_core.h"
#include <set>
#include <unordered_map>

static EPCVirtualProtect ext_process_vp_to_ep_core_vp(
    ExtProcess::VirtualProtect protection)
{
    EPCVirtualProtect epc_protection = EPCVP_EMPTY;
    switch (static_cast<uintptr_t>(protection))
    {
    case ExtProcess::VirtualProtect::READ | ExtProcess::VirtualProtect::WRITE |
        ExtProcess::VirtualProtect::EXECUTE:
    case ExtProcess::VirtualProtect::WRITE |
        ExtProcess::VirtualProtect::EXECUTE:
    {
        epc_protection = static_cast<EPCVirtualProtect>(
            EPCVP_READ | EPCVP_WRITE | EPCVP_EXECUTE);
        break;
    }
    case ExtProcess::VirtualProtect::READ | ExtProcess::VirtualProtect::EXECUTE:
    {
        epc_protection =
            static_cast<EPCVirtualProtect>(EPCVP_READ | EPCVP_EXECUTE);
        break;
    }
    case ExtProcess::VirtualProtect::READ | ExtProcess::VirtualProtect::WRITE:
    {
        epc_protection =
            static_cast<EPCVirtualProtect>(EPCVP_READ | EPCVP_WRITE);
        break;
    }
    case ExtProcess::VirtualProtect::READ:
    {
        epc_protection = EPCVP_READ;
        break;
    }
    case ExtProcess::VirtualProtect::WRITE:
    {
        epc_protection = EPCVP_WRITE;
        break;
    }
    default:
    {
        throw std::runtime_error("Invalid protection.");
        break;
    }
    }
    return epc_protection;
}

static EPCCallConvention ext_process_cc_to_ep_core_cc(
    ExtProcess::CallConvention convention)
{
    EPCCallConvention epc_convention = EPCCC_INVALID;
    switch (convention)
    {
    case ExtProcess::CallConvention::CDECL:
    {
        epc_convention = EPCCC_CDECL;
        break;
    }
    case ExtProcess::CallConvention::STDCALL:
    {
        epc_convention = EPCCC_STDCALL;
        break;
    }
    case ExtProcess::CallConvention::THISCALL:
    {
        epc_convention = EPCCC_THISCALL_MSVC;
        break;
    }
    case ExtProcess::CallConvention::FASTCALL:
    {
        epc_convention = EPCCC_FASTCALL;
        break;
    }
    default:
    {
        throw std::runtime_error("Invalid call convention.");
        break;
    }
    }
    return epc_convention;
}

class ExtProcess::Impl
{
public:
    Impl(unsigned int process_id)
    {
        init(process_id);
    }
    Impl(const std::string& process_name)
    {
        unsigned int process_id;
        if (!epc_get_process_id_by_name(process_name.c_str(), &process_id))
        {
            throw std::runtime_error("Failed to find process by name.");
        }
        init(process_id);
    }
    void init(unsigned int process_id)
    {
        if (!epc_create(&process_))
        {
            throw std::runtime_error("Failed to create EPCProcess.");
        }
        if (!epc_open_process(process_, process_id))
        {
            epc_destroy(process_);
            throw std::runtime_error("Failed to open process.");
        }
    }
    ~Impl()
    {
        /* Restore virtual protections */
        EPCVirtualProtect tmp_prot;
        for (auto& pair : virtual_protects_)
        {
            if (!epc_virtual_protect(process_, pair.first, pair.second.second,
                                     pair.second.first, &tmp_prot))
            {
                // throw std::runtime_error(
                // "Failed to restore virtual protection.");
            }
        }
        /* Free all allocated memory on destruction */
        for (auto address : allocated_addresses_)
        {
            epc_free(process_, address);
        }
        /* Destroy ep_core */
        if (process_)
        {
            epc_close_process(process_);
            epc_destroy(process_);
        }
    }

    uintptr_t alloc(size_t size)
    {
        uintptr_t address;
        if (!epc_alloc(process_, size, &address))
        {
            throw std::runtime_error("Memory allocation failed.");
        }
        allocated_addresses_.insert(address);
        return address;
    }

    void free(uintptr_t address)
    {
        if (allocated_addresses_.find(address) != allocated_addresses_.end())
        {
            if (!epc_free(process_, address))
            {
                throw std::runtime_error("Memory free failed.");
            }
            allocated_addresses_.erase(address);
        }
        else
        {
            throw std::runtime_error("Attempted to free unallocated memory.");
        }
    }

    void set_virtual_protect(uintptr_t address, size_t size,
                             ExtProcess::VirtualProtect protection)
    {
        EPCVirtualProtect old_prot;
        if (!epc_virtual_protect(process_, address, size,
                                 ext_process_vp_to_ep_core_vp(protection),
                                 &old_prot))
        {
            throw std::runtime_error("Failed to set virtual protection.");
        }
        virtual_protects_.insert(
            std::make_pair(address, std::make_pair(old_prot, size)));
    }

    void restore_virtual_protect(uintptr_t address)
    {
        auto it = virtual_protects_.find(address);
        if (it != virtual_protects_.end())
        {
            if (!epc_virtual_protect(process_, address, it->second.second,
                                     it->second.first, nullptr))
            {
                throw std::runtime_error(
                    "Failed to restore virtual protection.");
            }
            virtual_protects_.erase(it);
        }
        else
        {
            // throw std::runtime_error(
            //     "Attempted to restore virtual protection on unallocated "
            //     "memory.");
        }
    }
    uintptr_t call_function(uintptr_t address, CallConvention convention,
                            const std::vector<uintptr_t>& args,
                            bool wait_for_return)
    {
        auto caller_iterator = callers_.find(address);
        EPCFunctionCaller* epc_caller = nullptr;
        if (caller_iterator != callers_.end())
        {
            epc_caller = caller_iterator->second;
        }
        else
        {
            if (!epc_function_caller_build(
                    process_, address, ext_process_cc_to_ep_core_cc(convention),
                    args.size(), &epc_caller))
            {
                throw std::runtime_error("Failed to build function caller.");
            }
            callers_.insert(std::make_pair(address, epc_caller));
        }
        // TODO: Cache args.
        if (!epc_function_caller_send_args(epc_caller, args.size(),
                                           args.data()))
        {
            throw std::runtime_error("Failed to send function arguments.");
        }
        uint32_t result;
        if (!epc_function_caller_call(epc_caller, &result, wait_for_return))
        {
            throw std::runtime_error("Failed to call function.");
        }
        return result;
    }

    EPCProcess* process_;

private:
    std::set<uintptr_t> allocated_addresses_;
    std::unordered_map<uintptr_t, std::pair<EPCVirtualProtect, size_t>>
        virtual_protects_;
    std::unordered_map<uintptr_t, EPCFunctionCaller*> callers_;
};

ExtProcess::ExtProcess(unsigned int process_id)
    : p_impl_(std::make_unique<Impl>(process_id))
{
}

ExtProcess::ExtProcess(const std::string& process_name)
    : p_impl_(std::make_unique<Impl>(process_name))
{
}

ExtProcess::~ExtProcess()
{
}

uintptr_t ExtProcess::get_module_address(const char* module_name)
{
    uintptr_t address;
    if (!epc_get_module_address(p_impl_->process_, module_name, &address))
    {
        throw std::runtime_error("Failed to get module address.");
    }
    return address;
}

uintptr_t ExtProcess::alloc(size_t size) const
{
    return p_impl_->alloc(size);
}

void ExtProcess::free(uintptr_t address) const
{
    return p_impl_->free(address);
}

std::vector<uint8_t> ExtProcess::read_buf(const uintptr_t address,
                                          size_t size) const
{
    std::vector<uint8_t> buffer(size);
    if (!epc_read_buf(p_impl_->process_, address, size, buffer.data()))
    {
        throw std::runtime_error("Failed to read buffer.");
    }
    return buffer;
}
void ExtProcess::write_buf(uintptr_t address, const void* buf,
                           size_t size) const
{
    if (!epc_write_buf(p_impl_->process_, address, size, buf))
    {
        throw std::runtime_error("Failed to write buffer.");
    }
}

void ExtProcess::set_virtual_protect(
    uintptr_t address, size_t size, ExtProcess::VirtualProtect protection) const
{
    p_impl_->set_virtual_protect(address, size, protection);
}

void ExtProcess::restore_virtual_protect(uintptr_t address) const
{
    p_impl_->restore_virtual_protect(address);
}

uintptr_t ExtProcess::call_function(uintptr_t address,
                                    CallConvention convention,
                                    const std::vector<uintptr_t>& args,
                                    bool wait_for_return)
{
    return p_impl_->call_function(address, convention, args, wait_for_return);
}


bool ExtProcess::direct_read(uintptr_t address, void* buf, size_t size) const
{
    return epc_read_buf(p_impl_->process_, address, size, buf);
}
