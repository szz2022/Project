#ifdef _WIN32
    #include "WIN/WinSerialBase.h"
#elif __linux__
    #include "Linux/LinuxSerialBase.h"
#endif

#include "Serial.h"

using serial::Serial;
using serial::SerialException;
using serial::IOException;
using serial::bytesize_t;
using serial::parity_t;
using serial::parity_t;
using serial::stopbits_t;
using serial::flowcontrol_t;

class Serial::ScopedReadLock {
public:
	ScopedReadLock(SerialImpl* pimpl) : pimpl_(pimpl) {
		this->pimpl_->readLock();
	}
	~ScopedReadLock() {
		this->pimpl_->readUnlock();
	}
private:
	// Disable copy constructors
	ScopedReadLock(const ScopedReadLock&);
	const ScopedReadLock& operator=(ScopedReadLock);

	SerialImpl* pimpl_;
};

class Serial::ScopedWriteLock {
public:
	ScopedWriteLock(SerialImpl* pimpl) : pimpl_(pimpl) {
		this->pimpl_->writeLock();
	}
	~ScopedWriteLock() {
		this->pimpl_->writeUnlock();
	}
private:
	// Disable copy constructors
	ScopedWriteLock(const ScopedWriteLock&);
	const ScopedWriteLock& operator=(ScopedWriteLock);
	SerialImpl* pimpl_;
};

Serial::Serial(const std::string& port, unsigned int baudrate, serial::Timeout timeout, bytesize_t bytesize, parity_t parity, stopbits_t stopbits, flowcontrol_t flowcontrol) 
	: pimpl_(new SerialImpl(port, baudrate, bytesize, parity, stopbits, flowcontrol)) {
	pimpl_->setTimeout(timeout);
}

Serial::~Serial() {
	delete pimpl_;
}

void Serial::open() {
	pimpl_->open();
}

void Serial::close() {
	pimpl_->close();
}

bool Serial::isOpen() const {
	return pimpl_->isOpen();
}

unsigned int Serial::available() {
	return pimpl_->available();
}

bool Serial::waitReadable() {
	serial::Timeout timeout(pimpl_->getTimeout());
	return pimpl_->waitReadable(timeout.read_timeout_constant);
}

void Serial::waitByteTimes(unsigned int count) {
	pimpl_->waitByteTimes(count);
}

unsigned int Serial::read_(unsigned char* buffer, unsigned int size) {
	return this->pimpl_->read(buffer, size);
}

unsigned int Serial::read(unsigned char* buffer, unsigned int size) {
	ScopedReadLock lock(this->pimpl_);
	return this->pimpl_->read(buffer, size);
}

unsigned int Serial::read(std::vector<unsigned char>& buffer, unsigned int size) {
	ScopedReadLock lock(this->pimpl_);
	unsigned char* buffer_ = new unsigned char[size];
	unsigned int bytes_read = 0;

	try {
		bytes_read = this->pimpl_->read(buffer_, size);
	}
	catch (const std::exception& e) {
		delete[] buffer_;
		throw;
	}

	buffer.insert(buffer.end(), buffer_, buffer_ + bytes_read);
	delete[] buffer_;
	return bytes_read;
}

unsigned int Serial::read(std::string& buffer, unsigned int size) {
	ScopedReadLock lock(this->pimpl_);
	unsigned char* buffer_ = new unsigned char[size];
	unsigned int bytes_read = 0;
	try {
		bytes_read = this->pimpl_->read(buffer_, size);
	}
	catch (const std::exception& e) {
		delete[] buffer_;
		throw;
	}
	buffer.append(reinterpret_cast<const char*>(buffer_), bytes_read);
	delete[] buffer_;
	return bytes_read;
}

std::string Serial::read(unsigned int size) {
	std::string buffer;
	this->read(buffer, size);
	return buffer;
}

unsigned int Serial::readline(std::string& buffer, unsigned int size, std::string eol)
{
	ScopedReadLock lock(this->pimpl_);
	unsigned int eol_len = eol.length();
	unsigned char* buffer_ = static_cast<unsigned char*>(alloca(size * sizeof(unsigned char)));
	unsigned int read_so_far = 0;
	while (true) {
		unsigned int bytes_read = this->read_(buffer_ + read_so_far, 1);
		read_so_far += bytes_read;
		if (bytes_read == 0) {
			break; // Timeout occured on reading 1 byte
		}
		if (read_so_far < eol_len)
            continue;
		if (std::string(reinterpret_cast<const char*>
			(buffer_ + read_so_far - eol_len), eol_len) == eol) {
			break; // EOL found
		}
		if (read_so_far == size) {
			break; // Reached the maximum read length
		}
	}
	buffer.append(reinterpret_cast<const char*> (buffer_), read_so_far);
	return read_so_far;
}

std::string Serial::readline(unsigned int size, std::string eol) {
	std::string buffer;
	this->readline(buffer, size, eol);
	return buffer;
}

std::vector<std::string> Serial::readlines(unsigned int size, std::string eol) {
	ScopedReadLock lock(this->pimpl_);
	std::vector<std::string> lines;
	unsigned int eol_len = eol.length();
	unsigned char* buffer_ = static_cast<unsigned char*>(alloca(size * sizeof(unsigned char)));
	unsigned int read_so_far = 0;
	unsigned int start_of_line = 0;
	while (read_so_far < size) {
		unsigned int bytes_read = this->read_(buffer_ + read_so_far, 1);
		read_so_far += bytes_read;
		if (bytes_read == 0) {
			if (start_of_line != read_so_far) {
				lines.push_back(std::string(reinterpret_cast<const char*> (buffer_ + start_of_line), read_so_far - start_of_line));
			}
			break; // Timeout occured on reading 1 byte
		}
		if (read_so_far < eol_len)
            continue;

		if (std::string(reinterpret_cast<const char*>(buffer_ + read_so_far - eol_len), eol_len) == eol) {
			// EOL found
			lines.push_back(std::string(reinterpret_cast<const char*> (buffer_ + start_of_line), read_so_far - start_of_line));
			start_of_line = read_so_far;
		}
		if (read_so_far == size) {
			if (start_of_line != read_so_far) {
				lines.push_back(std::string(reinterpret_cast<const char*> (buffer_ + start_of_line), read_so_far - start_of_line));
			}
			break; // Reached the maximum read length
		}
	}
	return lines;
}

unsigned int Serial::write(const std::string& data) {
	ScopedWriteLock lock(this->pimpl_);
	return this->write_(reinterpret_cast<const unsigned char*>(data.c_str()), data.length());
}

unsigned int Serial::write(const std::vector<unsigned char>& data) {
	ScopedWriteLock lock(this->pimpl_);
	return this->write_(&data[0], data.size());
}

unsigned int Serial::write(const unsigned char* data, unsigned int size) {
	ScopedWriteLock lock(this->pimpl_);
	return this->write_(data, size);
}

unsigned int Serial::write_(const unsigned char* data, unsigned int length) {
	return pimpl_->write(data, length);
}

void Serial::setPort(const std::string& port) {
	ScopedReadLock rlock(this->pimpl_);
	ScopedWriteLock wlock(this->pimpl_);
	bool was_open = pimpl_->isOpen();
	if (was_open)
		close();
	pimpl_->setPort(port);
	if (was_open)
		open();
}

std::string Serial::getPort() const {
	return pimpl_->getPort();
}

void Serial::setTimeout(serial::Timeout& timeout) {
	pimpl_->setTimeout(timeout);
}

serial::Timeout Serial::getTimeout() const {
	return pimpl_->getTimeout();
}

void Serial::setBaudrate(unsigned int baudrate) {
	pimpl_->setBaudrate(baudrate);
}

unsigned int Serial::getBaudrate() const {
	return static_cast<unsigned int>(pimpl_->getBaudrate());
}

void Serial::setBytesize(bytesize_t bytesize) {
	pimpl_->setBytesize(bytesize);
}

bytesize_t Serial::getBytesize() const {
	return pimpl_->getBytesize();
}

void Serial::setParity(parity_t parity) {
	pimpl_->setParity(parity);
}

parity_t Serial::getParity() const {
	return pimpl_->getParity();
}

void Serial::setStopbits(stopbits_t stopbits) {
	pimpl_->setStopbits(stopbits);
}

stopbits_t Serial::getStopbits() const {
	return pimpl_->getStopbits();
}

void Serial::setFlowcontrol(flowcontrol_t flowcontrol) {
	pimpl_->setFlowcontrol(flowcontrol);
}

flowcontrol_t Serial::getFlowcontrol() const {
	return pimpl_->getFlowcontrol();
}

void Serial::flush() {
	ScopedReadLock rlock(this->pimpl_);
	ScopedWriteLock wlock(this->pimpl_);
	pimpl_->flush();
}

void Serial::flushInput() {
	ScopedReadLock lock(this->pimpl_);
	pimpl_->flushInput();
}

void Serial::flushOutput() {
	ScopedWriteLock lock(this->pimpl_);
	pimpl_->flushOutput();
}

void Serial::sendBreak(int duration) {
	pimpl_->sendBreak(duration);
}

void Serial::setBreak(bool level) {
	pimpl_->setBreak(level);
}

void Serial::setRTS(bool level) {
	pimpl_->setRTS(level);
}

void Serial::setDTR(bool level) {
	pimpl_->setDTR(level);
}

bool Serial::waitForChange() {
	return pimpl_->waitForChange();
}

bool Serial::getCTS() {
	return pimpl_->getCTS();
}

bool Serial::getDSR() {
	return pimpl_->getDSR();
}

bool Serial::getRI() {
	return pimpl_->getRI();
}

bool Serial::getCD() {
	return pimpl_->getCD();
}
