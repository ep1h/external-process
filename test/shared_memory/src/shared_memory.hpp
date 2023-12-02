#ifndef SHARED_MEMORY_HPP
#define SHARED_MEMORY_HPP

#include <string>
#include <stdexcept>
#include <windows.h>

template <typename T> class SharedMemory
{
public:
    SharedMemory(const std::string& name)
    {
        map_file_handle_ =
            CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
                              sizeof(T), name.c_str());

        if (map_file_handle_ == NULL)
        {
            throw std::runtime_error("Could not create file mapping object.");
        }

        buf_ptr_ = MapViewOfFile(map_file_handle_, FILE_MAP_ALL_ACCESS, 0, 0,
                                 sizeof(T));
        if (buf_ptr_ == NULL)
        {
            CloseHandle(map_file_handle_);
            throw std::runtime_error("Could not map view of file.");
        }
    }
    ~SharedMemory()
    {
        close_();
    }

    SharedMemory(const SharedMemory&) = delete;
    SharedMemory& operator=(const SharedMemory&) = delete;

    SharedMemory(SharedMemory&& other) noexcept
    {
        other.map_file_handle_ = NULL;
        other.buf_ptr_ = nullptr;
    }
    SharedMemory& operator=(SharedMemory&& other) noexcept
    {
        if (this != &other)
        {
            close_();
            map_file_handle_ = other.map_file_handle_;
            buf_ptr_ = other.buf_ptr_;
            m_size = other.m_size;

            other.map_file_handle_ = NULL;
            other.buf_ptr_ = nullptr;
        }
        return *this;
    }

    T* get(void) const
    {
        return reinterpret_cast<T*>(buf_ptr_);
    }

private:
    HANDLE map_file_handle_ = NULL;
    LPVOID buf_ptr_ = nullptr;
    size_t m_size;

    void close_(void)
    {
        if (buf_ptr_)
        {
            UnmapViewOfFile(buf_ptr_);
            buf_ptr_ = nullptr;
        }
        if (map_file_handle_)
        {
            CloseHandle(map_file_handle_);
            map_file_handle_ = NULL;
        }
    }
};

#endif /* SHARED_MEMORY_HPP */
