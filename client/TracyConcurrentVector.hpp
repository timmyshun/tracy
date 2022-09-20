#ifndef __TRACYCONCURRENTVECTOR_HPP__
#define __TRACYCONCURRENTVECTOR_HPP__

#include <shared_mutex>
#include <assert.h>
#include <atomic>
#include <functional>
#include <thread>
#include <chrono>
#include "../common/TracyAlloc.hpp"
#include "../common/TracyApi.h"

namespace tracy
{
template<typename T>
class ConcurrentVector
{
	using SharedLock = std::shared_mutex;
public:
	TRACY_API ConcurrentVector(size_t capacity)
		: m_ptr((T*)tracy_malloc(sizeof(T)* capacity))
		, m_tail(0)
		, m_capacity(capacity)
	{
		m_size.store(0);
		m_mallocStatus.store(false);
		assert(capacity != 0);
	}

	ConcurrentVector(const ConcurrentVector&) = delete;
	ConcurrentVector(ConcurrentVector&&) = delete;

	~ConcurrentVector()
	{
		tracy_free(m_ptr);
	}

	ConcurrentVector& operator=(const ConcurrentVector&) = delete;
	ConcurrentVector& operator=(ConcurrentVector&&) = delete;

	TRACY_API bool empty() const { return m_size.load(std::memory_order_relaxed) == 0; }
	TRACY_API size_t size() const { return m_size.load(std::memory_order_relaxed); }

	TRACY_API T* data() { return m_ptr; }
	TRACY_API const T* data() const { return m_ptr; };

	TRACY_API T* begin() { return m_ptr; }
	TRACY_API const T* begin() const { return m_ptr; }
	TRACY_API T* end() { return &m_ptr[m_tail]; }
	TRACY_API const T* end() const { return &m_ptr[m_tail]; }

	TRACY_API T& front() { assert(!empty()); return m_ptr[0]; }
	TRACY_API const T& front() const { assert(!empty()); return m_ptr[0]; }

	TRACY_API T& back() { assert(!empty()); return m_ptr[m_tail.load(std::memory_order_relaxed) - 1]; }
	TRACY_API const T& back() const { assert(!empty()); return m_ptr[m_tail.load(std::memory_order_relaxed) - 1]; }

	TRACY_API T& operator[](size_t idx) { return m_ptr[idx]; }
	TRACY_API const T& operator[](size_t idx) const { return m_ptr[idx]; }

	TRACY_API T* prepare_next(size_t count = 1)
	{
		m_lock.lock_shared();
		size_t index = m_tail.fetch_add(count);
		if ((index + count) >= m_capacity) [[unlikely]]
		{
			m_mallocStatus.store(true);
			m_lock.unlock_shared();
			AllocMore(count);
			m_lock.lock_shared();
			m_mallocStatus.store(false);
		}
		return &m_ptr[index];
	}

	TRACY_API void commit_next(size_t count = 1)
	{
		m_size.fetch_add(count);
		m_lock.unlock_shared();
	}

	TRACY_API void swap(ConcurrentVector& vec)
	{
		while (m_mallocStatus.load() == true)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		m_lock.lock();
		vec.m_lock.lock();

		assert(m_size == m_tail);
		assert(vec.m_size == vec.m_tail);

		// swap data
		const auto ptr1 = m_ptr;
		const auto ptr2 = vec.m_ptr;
		const auto size1 = m_size.load(std::memory_order_relaxed);
		const auto size2 = vec.m_size.load(std::memory_order_relaxed);
		const auto capacity1 = m_capacity;
		const auto capacity2 = vec.m_capacity;

		m_ptr = ptr2;
		vec.m_ptr = ptr1;
		m_size.store(size2, std::memory_order_relaxed);
		m_tail.store(size2, std::memory_order_relaxed);

		vec.m_size.store(size1, std::memory_order_relaxed);
		vec.m_tail.store(size1, std::memory_order_relaxed);

		m_capacity = capacity2;
		vec.m_capacity = capacity1;

		vec.m_lock.unlock();
		m_lock.unlock();
	}


	TRACY_API void clear()
	{
		m_lock.lock();
		m_tail.store(0, std::memory_order_relaxed);
		m_size.store(0, std::memory_order_relaxed);
		m_lock.unlock();
	}

	TRACY_API void iter_clear(std::function<void(const T&)> iter_func)
	{
		m_lock.lock();
		for (int i = 0; i < m_size; ++i)
		{
			iter_func(m_ptr[i]);
		}
		m_tail.store(0, std::memory_order_relaxed);
		m_size.store(0, std::memory_order_relaxed);
		m_lock.unlock();
	}

private:
	void AllocMore(size_t count)
	{
		m_lock.lock();
		size_t min_size = m_tail.load(std::memory_order_relaxed) + count;
		if (m_capacity < min_size)
		{
			m_capacity = std::max(m_capacity * 2, min_size);
			T* ptr = (T*)tracy_malloc(sizeof(T) * m_capacity);
			memcpy(ptr, m_ptr, m_size.load(std::memory_order_relaxed) * sizeof(T));
			tracy_free_fast(m_ptr);
			m_ptr = ptr;
		}
		m_lock.unlock();
	}

	T* m_ptr;

	SharedLock m_lock;
	std::atomic_size_t m_tail;
	std::atomic_size_t m_size;
	std::atomic_bool m_mallocStatus;
	size_t m_capacity;
};

}

#endif