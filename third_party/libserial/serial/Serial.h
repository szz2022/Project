#pragma once

#include <limits>
#include <vector>
#include <string>
#include <cstring>
#include <sstream>
#include <exception>
#include <stdexcept>

#define THROW(exceptionClass, message) throw exceptionClass( __FILE__, __LINE__, (message) )

namespace serial {
    typedef enum {
        fivebits = 5,
        sixbits = 6,
        sevenbits = 7,
        eightbits = 8
    } bytesize_t;

    typedef enum {
        parity_none = 0,
        parity_odd = 1,
        parity_even = 2,
        parity_mark = 3,
        parity_space = 4
    } parity_t;

    typedef enum {
        stopbits_one = 1,
        stopbits_two = 2,
        stopbits_one_point_five
    } stopbits_t;

    typedef enum {
        flowcontrol_none = 0,
        flowcontrol_software,
        flowcontrol_hardware
    } flowcontrol_t;

    struct Timeout {
        static unsigned int max() { return std::numeric_limits<unsigned int>::max(); }

        static Timeout simpleTimeout(unsigned int timeout) {
            return Timeout(max(), timeout, 0, timeout, 0);
        }

        unsigned int inter_byte_timeout;
        unsigned int read_timeout_constant;
        unsigned int read_timeout_multiplier;
        unsigned int write_timeout_constant;
        unsigned int write_timeout_multiplier;

        explicit Timeout(unsigned int inter_byte_timeout_ = 0,
                         unsigned int read_timeout_constant_ = 0,
                         unsigned int read_timeout_multiplier_ = 0,
                         unsigned int write_timeout_constant_ = 0,
                         unsigned int write_timeout_multiplier_ = 0)
                :
                inter_byte_timeout(inter_byte_timeout_),
                read_timeout_constant(read_timeout_constant_),
                read_timeout_multiplier(read_timeout_multiplier_),
                write_timeout_constant(write_timeout_constant_),
                write_timeout_multiplier(write_timeout_multiplier_)
        {}
    };

    class Serial {
    public:
        Serial(const std::string& port = "", unsigned int baudrate = 921600, Timeout timeout = Timeout(), bytesize_t bytesize = eightbits, parity_t parity = parity_none, stopbits_t stopbits = stopbits_one, flowcontrol_t flowcontrol = flowcontrol_none);

        virtual ~Serial();

        void open();

        bool isOpen() const;

        void close();

        unsigned int available();

        bool waitReadable();

        void waitByteTimes(unsigned int count);

        unsigned int read(unsigned char* buffer, unsigned int size);

        unsigned int read(std::vector<unsigned char>& buffer, unsigned int size = 1);

        unsigned int read(std::string& buffer, unsigned int size = 1);

        std::string read(unsigned int size = 1);

        unsigned int readline(std::string& buffer, unsigned int size = 65536, std::string eol = "\n");

        std::string readline(unsigned int size = 65536, std::string eol = "\n");

        std::vector<std::string> readlines(unsigned int size = 65536, std::string eol = "\n");

        unsigned int write(const unsigned char* data, unsigned int size);

        unsigned int write(const std::vector<unsigned char>& data);

        unsigned int write(const std::string& data);

        void setPort(const std::string& port);

        std::string getPort() const;

        void setTimeout(Timeout& timeout);

        void setTimeout(unsigned int inter_byte_timeout, unsigned int read_timeout_constant, unsigned int read_timeout_multiplier, unsigned int write_timeout_constant, unsigned int write_timeout_multiplier)
        {
            Timeout timeout(inter_byte_timeout, read_timeout_constant, read_timeout_multiplier, write_timeout_constant, write_timeout_multiplier);
            return setTimeout(timeout);
        }

        Timeout getTimeout() const;

        void setBaudrate(unsigned int baudrate);

        unsigned int getBaudrate() const;

        void setBytesize(bytesize_t bytesize);

        bytesize_t getBytesize() const;

        void setParity(parity_t parity);

        parity_t getParity() const;

        void setStopbits(stopbits_t stopbits);

        stopbits_t getStopbits() const;

        void setFlowcontrol(flowcontrol_t flowcontrol);

        flowcontrol_t getFlowcontrol() const;

        void flush();

        void flushInput();

        void flushOutput();

        void sendBreak(int duration);

        void setBreak(bool level = true);

        void setRTS(bool level = true);

        void setDTR(bool level = true);

        bool waitForChange();

        bool getCTS();

        bool getDSR();

        bool getRI();

        bool getCD();

    private:
        // Disable copy constructors
        Serial(const Serial&);
        Serial& operator=(const Serial&);

        // Pimpl idiom, d_pointer
        class SerialImpl;
        SerialImpl* pimpl_;

        // Scoped Lock Classes
        class ScopedReadLock;
        class ScopedWriteLock;

        // Read common function
        unsigned int read_(unsigned char* buffer, unsigned int size);
        // Write common function
        unsigned int write_(const unsigned char* data, unsigned int length);

    };

    // USER�Զ����쳣����
    class SerialException : public std::exception {
        // Disable copy constructors
        SerialException& operator=(const SerialException&);
        std::string e_what_;
    public:
        SerialException(const char* description) {
            std::stringstream ss;
            ss << "SerialException " << description << " failed.";
            e_what_ = ss.str();
        }
        SerialException(const SerialException& other) : e_what_(other.e_what_) {}
        virtual ~SerialException() throw() {}
        virtual const char* what() const throw () {
            return e_what_.c_str();
        }
    };

    class IOException : public std::exception {
        // Disable copy constructors
        IOException& operator = (const IOException&);
        std::string file_;
        int line_;
        std::string e_what_;
        int errno_;
    public:
        explicit IOException(std::string file, int line, int errnum) : file_(file), line_(line), errno_(errnum) {
            std::stringstream ss;
            #if defined(_WIN32) && !defined(__MINGW32__)
            char error_str [1024];
            strerror_s(error_str, 1024, errnum);
            #else
            char * error_str = strerror(errnum);
            #endif
            ss << "IO Exception (" << errno_ << "): " << error_str;
            ss << ", file " << file_ << ", line " << line_ << ".";
            e_what_ = ss.str();
        }
        explicit IOException(std::string file, int line, const char* description) : file_(file), line_(line), errno_(0) {
            std::stringstream ss;
            ss << "IO Exception: " << description;
            ss << ", file " << file_ << ", line " << line_ << ".";
            e_what_ = ss.str();
        }
        virtual ~IOException() throw() {}
        IOException(const IOException& other) : line_(other.line_), e_what_(other.e_what_), errno_(other.errno_) {}

        int getErrorNumber() const { return errno_; }

        virtual const char* what() const throw () {
            return e_what_.c_str();
        }
    };

    class PortNotOpenedException : public std::exception
    {
        // Disable copy constructors
        const PortNotOpenedException& operator=(PortNotOpenedException);
        std::string e_what_;
    public:
        PortNotOpenedException(const char* description) {
            std::stringstream ss;
            ss << "PortNotOpenedException " << description << " failed.";
            e_what_ = ss.str();
        }
        PortNotOpenedException(const PortNotOpenedException& other) : e_what_(other.e_what_) {}
        virtual ~PortNotOpenedException() throw() {}
        virtual const char* what() const throw () {
            return e_what_.c_str();
        }
    };

    struct PortInfo {
        std::string port;
        std::string description;
        std::string hardware_id;
    };
    std::vector<PortInfo>
    list_ports();
}
