#ifdef _WIN32
    #include "WinSerialBase.h"

    using serial::Serial;
    using serial::Timeout;
    using serial::bytesize_t;
    using serial::parity_t;
    using serial::stopbits_t;
    using serial::flowcontrol_t;

    using serial::SerialException;
    using serial::PortNotOpenedException;
    using serial::IOException;

    inline std::wstring _prefix_port_if_needed(const std::wstring& input)
    {
        static std::wstring windows_com_port_prefix = L"\\\\.\\";
        if (input.compare(0, windows_com_port_prefix.size(), windows_com_port_prefix) != 0) {
            return windows_com_port_prefix + input;
        }
        return input;
    }

    Serial::SerialImpl::SerialImpl(const std::string& port, unsigned long baudrate, bytesize_t bytesize, parity_t parity, stopbits_t stopbits, flowcontrol_t flowcontrol)
        :
        port_(port.begin(), port.end()),
        fd_(INVALID_HANDLE_VALUE),
        is_open_(false),
        baudrate_(baudrate),
        parity_(parity),
        bytesize_(bytesize),
        stopbits_(stopbits),
        flowcontrol_(flowcontrol)
    {
        if (port_.empty() == false)
            open();
        read_mutex = CreateMutex(NULL, false, NULL);
        write_mutex = CreateMutex(NULL, false, NULL);
    }

    Serial::SerialImpl::~SerialImpl()
    {
        this->close();
        CloseHandle(read_mutex);
        CloseHandle(write_mutex);
    }

    void Serial::SerialImpl::open()
    {
        if (port_.empty()) {
            throw std::invalid_argument("Empty port is invalid.");
        }
        if (is_open_ == true) {
            throw SerialException("Serial port already open.");
        }

        std::wstring port_with_prefix = _prefix_port_if_needed(port_);
        LPCWSTR lp_port = port_with_prefix.c_str();
        fd_ = CreateFileW(lp_port, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

        if (fd_ == INVALID_HANDLE_VALUE) {
            DWORD create_file_err = GetLastError();
            std::stringstream ss;
            switch (create_file_err) {
            case ERROR_FILE_NOT_FOUND:
                // Use this->getPort to convert to a std::std::string
                ss << "Specified port, " << this->getPort() << ", does not exist.";
                THROW(IOException, ss.str().c_str());
            default:
                ss << "Unknown error opening the serial port: " << create_file_err;
                THROW(IOException, ss.str().c_str());
            }
        }

        reconfigurePort();
        is_open_ = true;
    }

    void Serial::SerialImpl::reconfigurePort()
    {
        if (fd_ == INVALID_HANDLE_VALUE) {
            // Can only operate on a valid file descriptor
            THROW(IOException, "Invalid file descriptor, is the serial port open?");
        }

        DCB dcbSerialParams = { 0 };

        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

        SetCommMask(fd_, EV_RXCHAR);
        SetupComm(fd_, 524288, 524288);

        if (!GetCommState(fd_, &dcbSerialParams)) {
            //error getting state
            THROW(IOException, "Error getting the serial port state.");
        }

        // setup baud rate
        switch (baudrate_) {
        case 0: dcbSerialParams.BaudRate = 0; break;
        case 50: dcbSerialParams.BaudRate = 50; break;
        case 75: dcbSerialParams.BaudRate = 75; break;
        case 110: dcbSerialParams.BaudRate = CBR_110; break;
        case 134: dcbSerialParams.BaudRate = 134; break;
        case 150: dcbSerialParams.BaudRate = 150; break;
        case 200: dcbSerialParams.BaudRate = 200; break;
        case 300: dcbSerialParams.BaudRate = CBR_300; break;
        case 600: dcbSerialParams.BaudRate = CBR_600; break;
        case 1200: dcbSerialParams.BaudRate = CBR_1200; break;
        case 1800: dcbSerialParams.BaudRate = 1800; break;
        case 2400: dcbSerialParams.BaudRate = CBR_2400; break;
        case 4800: dcbSerialParams.BaudRate = CBR_4800; break;
        case 7200: dcbSerialParams.BaudRate = 7200; break;
        case 9600: dcbSerialParams.BaudRate = CBR_9600; break;
        case 14400: dcbSerialParams.BaudRate = CBR_14400; break;
        case 19200: dcbSerialParams.BaudRate = CBR_19200; break;
        case 28800: dcbSerialParams.BaudRate = 28800; break;
        case 57600: dcbSerialParams.BaudRate = CBR_57600; break;
        case 76800: dcbSerialParams.BaudRate = 76800; break;
        case 38400: dcbSerialParams.BaudRate = CBR_38400; break;
        case 115200: dcbSerialParams.BaudRate = CBR_115200; break;
        case 128000: dcbSerialParams.BaudRate = CBR_128000; break;
        case 153600: dcbSerialParams.BaudRate = 153600; break;
        case 230400: dcbSerialParams.BaudRate = 230400; break;
        case 256000: dcbSerialParams.BaudRate = CBR_256000; break;
        case 460800: dcbSerialParams.BaudRate = 460800; break;
        case 921600: dcbSerialParams.BaudRate = 921600; break;
        default:
            // Try to blindly assign it
            dcbSerialParams.BaudRate = baudrate_;
        }

        // setup char len
        if (bytesize_ == eightbits)
            dcbSerialParams.ByteSize = 8;
        else if (bytesize_ == sevenbits)
            dcbSerialParams.ByteSize = 7;
        else if (bytesize_ == sixbits)
            dcbSerialParams.ByteSize = 6;
        else if (bytesize_ == fivebits)
            dcbSerialParams.ByteSize = 5;
        else
            throw std::invalid_argument("invalid char len");

        // setup stopbits
        if (stopbits_ == stopbits_one)
            dcbSerialParams.StopBits = ONESTOPBIT;
        else if (stopbits_ == stopbits_one_point_five)
            dcbSerialParams.StopBits = ONE5STOPBITS;
        else if (stopbits_ == stopbits_two)
            dcbSerialParams.StopBits = TWOSTOPBITS;
        else
            throw std::invalid_argument("invalid stop bit");

        // setup parity
        if (parity_ == parity_none)
            dcbSerialParams.Parity = NOPARITY;
        else if (parity_ == parity_even)
            dcbSerialParams.Parity = EVENPARITY;
        else if (parity_ == parity_odd)
            dcbSerialParams.Parity = ODDPARITY;
        else if (parity_ == parity_mark)
            dcbSerialParams.Parity = MARKPARITY;
        else if (parity_ == parity_space)
            dcbSerialParams.Parity = SPACEPARITY;
        else
            throw std::invalid_argument("invalid parity");

        // setup flowcontrol
        if (flowcontrol_ == flowcontrol_none) {
            dcbSerialParams.fOutxCtsFlow = false;
            dcbSerialParams.fRtsControl = RTS_CONTROL_DISABLE;
            dcbSerialParams.fOutX = false;
            dcbSerialParams.fInX = false;
        }
        if (flowcontrol_ == flowcontrol_software) {
            dcbSerialParams.fOutxCtsFlow = false;
            dcbSerialParams.fRtsControl = RTS_CONTROL_DISABLE;
            dcbSerialParams.fOutX = true;
            dcbSerialParams.fInX = true;
        }
        if (flowcontrol_ == flowcontrol_hardware) {
            dcbSerialParams.fOutxCtsFlow = true;
            dcbSerialParams.fRtsControl = RTS_CONTROL_HANDSHAKE;
            dcbSerialParams.fOutX = false;
            dcbSerialParams.fInX = false;
        }

        // activate settings
        if (!SetCommState(fd_, &dcbSerialParams)) {
            CloseHandle(fd_);
            THROW(IOException, "Error setting serial port settings.");
        }

        // Setup timeouts
        COMMTIMEOUTS timeouts = { 0 };
        timeouts.ReadIntervalTimeout = timeout_.inter_byte_timeout;
        timeouts.ReadTotalTimeoutConstant = timeout_.read_timeout_constant;
        timeouts.ReadTotalTimeoutMultiplier = timeout_.read_timeout_multiplier;
        timeouts.WriteTotalTimeoutConstant = timeout_.write_timeout_constant;
        timeouts.WriteTotalTimeoutMultiplier = timeout_.write_timeout_multiplier;
        if (!SetCommTimeouts(fd_, &timeouts))
            THROW(IOException, "Error setting timeouts.");

        PurgeComm(fd_, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
    }

    void Serial::SerialImpl::close()
    {
        if (is_open_ == true) {
            if (fd_ != INVALID_HANDLE_VALUE) {
                int ret;
                ret = CloseHandle(fd_);
                if (ret == 0) {
                    std::stringstream ss;
                    ss << "Error while closing serial port: " << GetLastError();
                    THROW(IOException, ss.str().c_str());
                }
                else {
                    fd_ = INVALID_HANDLE_VALUE;
                }
            }
            is_open_ = false;
        }
    }

    bool Serial::SerialImpl::isOpen() const
    {
        return is_open_;
    }

    unsigned int Serial::SerialImpl::available()
    {
        if (!is_open_) {
            return 0;
        }
        COMSTAT cs;
        if (!ClearCommError(fd_, NULL, &cs)) {
            std::stringstream ss;
            ss << "Error while checking status of the serial port: " << GetLastError();
            THROW(IOException, ss.str().c_str());
        }
        return static_cast<unsigned int>(cs.cbInQue);
    }

    bool Serial::SerialImpl::waitReadable(unsigned int /*timeout*/)
    {
        THROW(IOException, "waitReadable is not implemented on Windows.");
        return false;
    }

    void Serial::SerialImpl::waitByteTimes(unsigned int /*count*/)
    {
        THROW(IOException, "waitByteTimes is not implemented on Windows.");
    }

    //unsigned int Serial::SerialImpl::read(unsigned char* buf, unsigned int size)
    //{
    //	if (!is_open_) {
    //		throw PortNotOpenedException("Serial::read");
    //	}
    //	DWORD bytes_read;
    //	if (!ReadFile(fd_, buf, static_cast<DWORD>(size), &bytes_read, NULL)) {
    //		std::stringstream ss;
    //		ss << "Error while reading from the serial port: " << GetLastError();
    //		THROW(IOException, ss.str().c_str());
    //	}
    //	return (size_t)(bytes_read);
    //}

    unsigned int Serial::SerialImpl::read(unsigned char* buf, unsigned int size)
    {
        if (!is_open_) {
            throw PortNotOpenedException("Serial::read");
        }

        DWORD comm_error = 0;
        DWORD bytes_read = 0;
        COMSTAT com_stat{};

        if (ClearCommError(fd_, &comm_error, &com_stat) && comm_error > 0) {
            printf_s("error\n");
            return (unsigned int)(bytes_read);
        }
        if (com_stat.cbInQue >= size) {
            if (!ReadFile(fd_, buf, static_cast<DWORD>(size), &bytes_read, NULL)) {
                std::stringstream ss;
                ss << "Error while reading from the serial port: " << GetLastError();
                THROW(IOException, ss.str().c_str());
            }
        }
        return (unsigned int)(bytes_read);
    }

    unsigned int Serial::SerialImpl::write(const unsigned char* data, unsigned int length)
    {
        if (is_open_ == false) {
            throw PortNotOpenedException("Serial::write");
        }
        DWORD bytes_written;
        if (!WriteFile(fd_, data, static_cast<DWORD>(length), &bytes_written, NULL)) {
            std::stringstream ss;
            ss << "Error while writing to the serial port: " << GetLastError();
            THROW(IOException, ss.str().c_str());
        }
        return (unsigned int)(bytes_written);
    }

    void Serial::SerialImpl::setPort(const std::string& port)
    {
        port_ = std::wstring(port.begin(), port.end());
    }

    std::string Serial::SerialImpl::getPort() const
    {
        return std::string(port_.begin(), port_.end());
    }

    void Serial::SerialImpl::setTimeout(serial::Timeout& timeout)
    {
        timeout_ = timeout;
        if (is_open_) {
            reconfigurePort();
        }
    }

    serial::Timeout Serial::SerialImpl::getTimeout() const
    {
        return timeout_;
    }

    void Serial::SerialImpl::setBaudrate(unsigned long baudrate)
    {
        baudrate_ = baudrate;
        if (is_open_) {
            reconfigurePort();
        }
    }

    unsigned long Serial::SerialImpl::getBaudrate() const
    {
        return baudrate_;
    }

    void Serial::SerialImpl::setBytesize(serial::bytesize_t bytesize)
    {
        bytesize_ = bytesize;
        if (is_open_) {
            reconfigurePort();
        }
    }

    serial::bytesize_t Serial::SerialImpl::getBytesize() const
    {
        return bytesize_;
    }

    void Serial::SerialImpl::setParity(serial::parity_t parity)
    {
        parity_ = parity;
        if (is_open_) {
            reconfigurePort();
        }
    }

    serial::parity_t Serial::SerialImpl::getParity() const
    {
        return parity_;
    }

    void Serial::SerialImpl::setStopbits(serial::stopbits_t stopbits)
    {
        stopbits_ = stopbits;
        if (is_open_) {
            reconfigurePort();
        }
    }

    serial::stopbits_t Serial::SerialImpl::getStopbits() const
    {
        return stopbits_;
    }

    void Serial::SerialImpl::setFlowcontrol(serial::flowcontrol_t flowcontrol)
    {
        flowcontrol_ = flowcontrol;
        if (is_open_) {
            reconfigurePort();
        }
    }

    serial::flowcontrol_t Serial::SerialImpl::getFlowcontrol() const
    {
        return flowcontrol_;
    }

    void Serial::SerialImpl::flush()
    {
        if (is_open_ == false) {
            throw PortNotOpenedException("Serial::flush");
        }
        FlushFileBuffers(fd_);
    }

    void Serial::SerialImpl::flushInput()
    {
        if (is_open_ == false) {
            throw PortNotOpenedException("Serial::flushInput");
        }
        PurgeComm(fd_, PURGE_RXCLEAR);
    }

    void Serial::SerialImpl::flushOutput()
    {
        if (is_open_ == false) {
            throw PortNotOpenedException("Serial::flushOutput");
        }
        PurgeComm(fd_, PURGE_TXCLEAR);
    }

    void Serial::SerialImpl::sendBreak(int /*duration*/)
    {
        THROW(IOException, "sendBreak is not supported on Windows.");
    }

    void Serial::SerialImpl::setBreak(bool level)
    {
        if (is_open_ == false) {
            throw PortNotOpenedException("Serial::setBreak");
        }
        if (level) {
            EscapeCommFunction(fd_, SETBREAK);
        }
        else {
            EscapeCommFunction(fd_, CLRBREAK);
        }
    }

    void Serial::SerialImpl::setRTS(bool level)
    {
        if (is_open_ == false) {
            throw PortNotOpenedException("Serial::setRTS");
        }
        if (level) {
            EscapeCommFunction(fd_, SETRTS);
        }
        else {
            EscapeCommFunction(fd_, CLRRTS);
        }
    }

    void Serial::SerialImpl::setDTR(bool level)
    {
        if (is_open_ == false) {
            throw PortNotOpenedException("Serial::setDTR");
        }
        if (level) {
            EscapeCommFunction(fd_, SETDTR);
        }
        else {
            EscapeCommFunction(fd_, CLRDTR);
        }
    }

    bool Serial::SerialImpl::waitForChange()
    {
        if (is_open_ == false) {
            throw PortNotOpenedException("Serial::waitForChange");
        }
        DWORD dwCommEvent;

        if (!SetCommMask(fd_, EV_CTS | EV_DSR | EV_RING | EV_RLSD)) {
            // Error setting communications mask
            return false;
        }

        if (!WaitCommEvent(fd_, &dwCommEvent, NULL)) {
            // An error occurred waiting for the event.
            return false;
        }
        else {
            // Event has occurred.
            return true;
        }
    }

    bool Serial::SerialImpl::getCTS()
    {
        if (is_open_ == false) {
            throw PortNotOpenedException("Serial::getCTS");
        }
        DWORD dwModemStatus;
        if (!GetCommModemStatus(fd_, &dwModemStatus)) {
            THROW(IOException, "Error getting the status of the CTS line.");
        }

        return (MS_CTS_ON & dwModemStatus) != 0;
    }

    bool Serial::SerialImpl::getDSR()
    {
        if (is_open_ == false) {
            throw PortNotOpenedException("Serial::getDSR");
        }
        DWORD dwModemStatus;
        if (!GetCommModemStatus(fd_, &dwModemStatus)) {
            THROW(IOException, "Error getting the status of the DSR line.");
        }

        return (MS_DSR_ON & dwModemStatus) != 0;
    }

    bool Serial::SerialImpl::getRI()
    {
        if (is_open_ == false) {
            throw PortNotOpenedException("Serial::getRI");
        }
        DWORD dwModemStatus;
        if (!GetCommModemStatus(fd_, &dwModemStatus)) {
            THROW(IOException, "Error getting the status of the RI line.");
        }

        return (MS_RING_ON & dwModemStatus) != 0;
    }

    bool Serial::SerialImpl::getCD()
    {
        if (is_open_ == false) {
            throw PortNotOpenedException("Serial::getCD");
        }
        DWORD dwModemStatus;
        if (!GetCommModemStatus(fd_, &dwModemStatus)) {
            // Error in GetCommModemStatus;
            THROW(IOException, "Error getting the status of the CD line.");
        }

        return (MS_RLSD_ON & dwModemStatus) != 0;
    }

    void Serial::SerialImpl::readLock()
    {
        if (WaitForSingleObject(read_mutex, INFINITE) != WAIT_OBJECT_0) {
            THROW(IOException, "Error claiming read mutex.");
        }
    }

    void Serial::SerialImpl::readUnlock()
    {
        if (!ReleaseMutex(read_mutex)) {
            THROW(IOException, "Error releasing read mutex.");
        }
    }

    void Serial::SerialImpl::writeLock()
    {
        if (WaitForSingleObject(write_mutex, INFINITE) != WAIT_OBJECT_0) {
            THROW(IOException, "Error claiming write mutex.");
        }
    }

    void Serial::SerialImpl::writeUnlock()
    {
        if (!ReleaseMutex(write_mutex)) {
            THROW(IOException, "Error releasing write mutex.");
        }
    }

    std::vector<PortInfo> serial::list_ports()
    {
        std::vector<PortInfo> devices_found;

        HDEVINFO device_info_set = SetupDiGetClassDevs(
            (const GUID*)&GUID_DEVCLASS_PORTS,
            NULL,
            NULL,
            DIGCF_PRESENT);

        unsigned int device_info_set_index = 0;
        SP_DEVINFO_DATA device_info_data;

        device_info_data.cbSize = sizeof(SP_DEVINFO_DATA);

        while (SetupDiEnumDeviceInfo(device_info_set, device_info_set_index, &device_info_data))
        {
            device_info_set_index++;

            // Get port name

            HKEY hkey = SetupDiOpenDevRegKey(
                device_info_set,
                &device_info_data,
                DICS_FLAG_GLOBAL,
                0,
                DIREG_DEV,
                KEY_READ);

            TCHAR port_name[port_name_max_length];
            DWORD port_name_length = port_name_max_length;

            LONG return_code = RegQueryValueEx(
                hkey,
                _T("PortName"),
                NULL,
                NULL,
                (LPBYTE)port_name,
                &port_name_length);

            RegCloseKey(hkey);

            if (return_code != EXIT_SUCCESS)
                continue;

            if (port_name_length > 0 && port_name_length <= port_name_max_length)
                port_name[port_name_length - 1] = '\0';
            else
                port_name[0] = '\0';

            // Ignore parallel ports

            if (_tcsstr(port_name, _T("LPT")) != NULL)
                continue;

            // Get port friendly name

            TCHAR friendly_name[friendly_name_max_length]{};
            DWORD friendly_name_actual_length = 0;

            BOOL got_friendly_name = SetupDiGetDeviceRegistryProperty(
                device_info_set,
                &device_info_data,
                SPDRP_FRIENDLYNAME,
                NULL,
                (PBYTE)friendly_name,
                friendly_name_max_length,
                &friendly_name_actual_length);

            if (got_friendly_name == TRUE && friendly_name_actual_length > 0)
                friendly_name[friendly_name_actual_length - 1] = '\0';
            else
                friendly_name[0] = '\0';

            // Get hardware ID

            TCHAR hardware_id[hardware_id_max_length]{};
            DWORD hardware_id_actual_length = 0;

            BOOL got_hardware_id = SetupDiGetDeviceRegistryProperty(
                device_info_set,
                &device_info_data,
                SPDRP_HARDWAREID,
                NULL,
                (PBYTE)hardware_id,
                hardware_id_max_length,
                &hardware_id_actual_length);

            if (got_hardware_id == TRUE && hardware_id_actual_length > 0)
                hardware_id[hardware_id_actual_length - 1] = '\0';
            else
                hardware_id[0] = '\0';

            std::string portName = port_name;
            std::string friendlyName = friendly_name;
            std::string hardwareId = hardware_id;

            PortInfo port_entry;
            port_entry.port = portName;
            port_entry.description = friendlyName;
            port_entry.hardware_id = hardwareId;

            devices_found.push_back(port_entry);
        }

        SetupDiDestroyDeviceInfoList(device_info_set);

        return devices_found;
    }

#endif