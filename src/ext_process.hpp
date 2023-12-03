#ifndef EXT_PROCESS_HPP
#define EXT_PROCESS_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>

class ExtProcess
{
public:
    enum CallConvention
    {
        CDECL,
        STDCALL,
        THISCALL,
        FASTCALL
    };
    enum VirtualProtect
    {
        READ = 0b1,
        WRITE = 0b10,
        EXECUTE = 0b100,
    };
    explicit ExtProcess(unsigned int process_id);
    explicit ExtProcess(const std::string& process_name);
    ~ExtProcess();
    ExtProcess(const ExtProcess&) = delete;
    ExtProcess& operator=(const ExtProcess&) = delete;
    uintptr_t get_module_address(const char* module_name);
    uintptr_t alloc(size_t size) const;
    void free(uintptr_t address) const;
    std::vector<uint8_t> read_buf(const uintptr_t address,
                                         size_t size) const;
    void write_buf(const uintptr_t address, const void* buf,
                          const size_t size) const;
    void set_virtual_protect(uintptr_t address, size_t size,
                                    VirtualProtect protection) const;
    void restore_virtual_protect(uintptr_t address) const;
    uintptr_t call_function(uintptr_t address, CallConvention convention,
                            const std::vector<uintptr_t>& args,
                            bool wait_for_return = true);
    template <typename T> T read(uintptr_t address) const;
    template <typename T> void write(uintptr_t address, const T& data) const;

private:
    class Impl;
    std::unique_ptr<Impl> p_impl_;
    bool direct_read(uintptr_t address, void* buf, size_t size) const;
};

template <typename T> T ExtProcess::read(uintptr_t address) const
{
    static_assert(std::is_trivially_copyable<T>::value,
                  "Type T must be trivially copyable");
    T result;
    if (!direct_read(address, &result, sizeof(T)))
    {
        throw std::runtime_error("Failed to read memory.");
    }
    return result;
}

template <typename T>
void ExtProcess::write(uintptr_t address, const T& data) const
{
    static_assert(std::is_trivially_copyable<T>::value,
                  "Type T must be trivially copyable");

    write_buf(address, &data, sizeof(T));
}

#endif /* EXT_PROCESS_HPP */
