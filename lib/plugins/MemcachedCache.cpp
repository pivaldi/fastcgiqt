#include "MemcachedCache.h"

#include <libmemcached/memcached.h>

#include <QtEndian>
#include <QCoreApplication>
#include <QDebug>
#include <QReadLocker>
#include <QSettings>
#include <QStringList>
#include <QWriteLocker>

namespace FastCgiQt
{
	QReadWriteLock MemcachedCache::m_lock(QReadWriteLock::Recursive);

	MemcachedCache::MemcachedCache(const QString& cacheName)
		:
			m_memcached(memcached_create(NULL)),
			m_keyPrefix(cacheName + "::")
	{
		memcached_return rt;

		rt = memcached_server_add(m_memcached, "localhost", MEMCACHED_DEFAULT_PORT);
		Q_ASSERT(rt == MEMCACHED_SUCCESS);
	}
	
	MemcachedCache::~MemcachedCache()
	{
		memcached_free(m_memcached);
	}

	CacheBackend* MemcachedCacheFactory::getCacheBackend(const QString& cacheName) const
	{
		return new MemcachedCache(cacheName);
	}

	bool MemcachedCacheFactory::loadSettings()
	{
		QSettings settings(
			"." + QCoreApplication::applicationFilePath().split('/').last(),
			QSettings::IniFormat
		);
		return false;
	}

	CacheEntry MemcachedCache::value(const QString& key) const
	{
		QReadLocker lock(&m_lock);

		const QByteArray rawKey(key.toUtf8());

		char* rawData;
		size_t rawDataLength;
		uint32_t flags;
		memcached_return rt;

		rawData = memcached_get(
			m_memcached,
			rawKey.constData(),
			rawKey.length(),
			&rawDataLength,
			&flags,
			&rt
		);

		Q_ASSERT(rawDataLength >= sizeof(quint64));
		if(rawDataLength < sizeof(quint64) || !rawData)
		{
			return CacheEntry();
		}
		
		const quint64 timeStamp = qFromLittleEndian(*reinterpret_cast<quint64*>(rawData));
		const QByteArray data(rawData + sizeof(quint64), rawDataLength - sizeof(quint64));
		return CacheEntry(QDateTime::fromTime_t(timeStamp), data);
	}

	QReadWriteLock* MemcachedCache::readWriteLock() const
	{
		return &m_lock;
	}

	void MemcachedCache::setValue(const QString& key, const CacheEntry& entry)
	{
		QWriteLocker lock(&m_lock);

		// Binary key
		const QByteArray rawKey(key.toUtf8());

		// Binary data
		const quint64 dateTime(qToLittleEndian(static_cast<quint64>(entry.timeStamp().toTime_t())));
		QByteArray rawData(reinterpret_cast<const char*>(&dateTime), sizeof(quint64));
		rawData.append(entry.data());

		// Store in memcached
		memcached_return rt;
		rt = memcached_set(
			m_memcached,
			rawKey.constData(),
			rawKey.length(),
			rawData.constData(),
			rawData.length(),
			0, // expire
			0 // flags
		);
		Q_ASSERT(rt == MEMCACHED_SUCCESS);
	}

	void MemcachedCache::remove(const QString& key)
	{
		Q_UNUSED(key);
		QWriteLocker lock(&m_lock);
	}
}

Q_EXPORT_PLUGIN2(FastCgiQt_CacheBackend_MemcachedCache, FastCgiQt::MemcachedCacheFactory)
