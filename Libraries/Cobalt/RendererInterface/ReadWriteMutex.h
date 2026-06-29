// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_set>
namespace cobalt { namespace graphics {

class ReadWriteMutex
{
public:
	// Nested types
	class ReadLock
	{
	public:
		// Friend declarations
		friend class ReadWriteMutex;

	public:
		// Constructors
		inline ReadLock(ReadLock&& source) = default;
		inline ~ReadLock();

		// Lock methods
		inline void Unlock();

	protected:
		// Constructors
		inline ReadLock(ReadWriteMutex& mutex, bool firstLockOnThread);

	private:
		ReadWriteMutex& _mutex;
		bool _locked;
		bool _firstLockOnThread;
	};
	class WriteLock
	{
	public:
		// Friend declarations
		friend class ReadWriteMutex;

	public:
		// Constructors
		inline WriteLock(WriteLock&& source) = default;
		inline ~WriteLock();

		// Lock methods
		inline void Unlock();

	protected:
		// Constructors
		inline explicit WriteLock(ReadWriteMutex& mutex);

	private:
		ReadWriteMutex& _mutex;
		bool _locked;
	};

public:
	// Constructors
	inline ReadWriteMutex();

	// Lock methods
	inline std::unique_ptr<ReadLock> ObtainReadLock();
	inline std::unique_ptr<WriteLock> ObtainWriteLock();

private:
	std::mutex _shared;
	std::unordered_set<std::thread::id> _activeReadLockThreads;
	std::condition_variable _readLockReleased;
	std::condition_variable _writeLockReleased;
	int _activeReaderCount;
	int _totalWriterCount;
	int _activeWriterCount;
};

}} // namespace cobalt::graphics
#include "ReadWriteMutex.inl"
