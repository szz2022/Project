#ifdef _WIN32
    #pragma once

    #include "WinSerial.h"
    #include "windows.h"

    namespace serial {

        using serial::SerialException;
        using serial::IOException;

        class serial::Serial::SerialImpl {
        public:
            SerialImpl(const std::string& port, unsigned long baudrate, bytesize_t bytesize, parity_t parity, stopbits_t stopbits, flowcontrol_t flowcontrol);

            virtual ~SerialImpl();

            void open();

            void close();

            bool isOpen() const;

            unsigned int available();

            bool waitReadable(unsigned int timeout);

            void waitByteTimes(unsigned int count);

            unsigned int read(unsigned char* buf, unsigned int size = 1);

            unsigned int write(const unsigned char* data, unsigned int length);

            void flush();

            void flushInput();

            void flushOutput();

            void sendBreak(int duration);

            void setBreak(bool level);

            void setRTS(bool level);

            void setDTR(bool level);

            bool waitForChange();

            bool getCTS();

            bool getDSR();

            bool getRI();

            bool getCD();

            void setPort(const std::string& port);

            std::string getPort() const;

            void setTimeout(Timeout& timeout);

            Timeout getTimeout() const;

            void setBaudrate(unsigned long baudrate);

            unsigned long getBaudrate() const;

            void setBytesize(bytesize_t bytesize);

            bytesize_t getBytesize() const;

            void setParity(parity_t parity);

            parity_t getParity() const;

            void setStopbits(stopbits_t stopbits);

            stopbits_t getStopbits() const;

            void setFlowcontrol(flowcontrol_t flowcontrol);

            flowcontrol_t getFlowcontrol() const;

            void readLock();

            void readUnlock();

            void writeLock();

            void writeUnlock();

        protected:
            void reconfigurePort();

        private:
            std::wstring port_;
            HANDLE fd_;

            bool is_open_;

            Timeout timeout_;
            unsigned long baudrate_;

            parity_t parity_;
            bytesize_t bytesize_;
            stopbits_t stopbits_;
            flowcontrol_t flowcontrol_;

            HANDLE read_mutex;
            HANDLE write_mutex;
        };
    }
#endif
