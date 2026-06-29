// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// ReadLock methods
//----------------------------------------------------------------------------------------
ReadWriteMutex::ReadLock::ReadLock(ReadWriteMutex& mutex, bool firstLockOnThread)
: _mutex(mutex), _firstLockOnThread(firstLockOnThread), _locked(true)
{}

//----------------------------------------------------------------------------------------
ReadWriteMutex::ReadLock::~ReadLock()
{
	Unlock();
}

//----------------------------------------------------------------------------------------
void ReadWriteMutex::ReadLock::Unlock()
{
	if (!_locked)
	{
		return;
	}

	std::unique_lock<std::mutex> lock(_mutex._shared);
	--_mutex._activeReaderCount;
	if (_firstLockOnThread)
	{
		_mutex._activeReadLockThreads.erase(std::this_thread::get_id());
	}
	lock.unlock();
	_mutex._readLockReleased.notify_one();
	_locked = false;
}

//----------------------------------------------------------------------------------------
// WriteLock methods
//----------------------------------------------------------------------------------------
ReadWriteMutex::WriteLock::WriteLock(ReadWriteMutex& mutex)
: _mutex(mutex), _locked(true)
{}

//----------------------------------------------------------------------------------------
ReadWriteMutex::WriteLock::~WriteLock()
{
	Unlock();
}

//----------------------------------------------------------------------------------------
void ReadWriteMutex::WriteLock::Unlock()
{
	if (!_locked)
	{
		return;
	}

	std::unique_lock<std::mutex> lock(_mutex._shared);
	--_mutex._totalWriterCount;
	--_mutex._activeWriterCount;
	if (_mutex._totalWriterCount > 0)
	{
		_mutex._readLockReleased.notify_one();
	}
	else
	{
		_mutex._writeLockReleased.notify_all();
	}
	lock.unlock();
	_locked = false;
}

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
ReadWriteMutex::ReadWriteMutex()
: _activeReaderCount(0), _totalWriterCount(0), _activeWriterCount(0)
{}

//----------------------------------------------------------------------------------------
// Lock methods
//----------------------------------------------------------------------------------------
std::unique_ptr<ReadWriteMutex::ReadLock> ReadWriteMutex::ObtainReadLock()
{
	std::unique_lock<std::mutex> lock(_shared);
	auto currentThreadID = std::this_thread::get_id();
	bool firstLockOnThread = _activeReadLockThreads.find(currentThreadID) == _activeReadLockThreads.end();
	if (firstLockOnThread)
	{
		while (_totalWriterCount != 0)
		{
			_writeLockReleased.wait(lock);
		}
		_activeReadLockThreads.insert(currentThreadID);
	}
	++_activeReaderCount;
	lock.unlock();
	return std::unique_ptr<ReadLock>(new ReadLock(*this, firstLockOnThread));
}

//----------------------------------------------------------------------------------------
std::unique_ptr<ReadWriteMutex::WriteLock> ReadWriteMutex::ObtainWriteLock()
{
	std::unique_lock<std::mutex> lock(_shared);
	++_totalWriterCount;
	while ((_activeReaderCount != 0) || (_activeWriterCount != 0))
	{
		_readLockReleased.wait(lock);
	}
	++_activeWriterCount;
	lock.unlock();
	return std::unique_ptr<WriteLock>(new WriteLock(*this));
}

}} // namespace cobalt::graphics
