#ifndef _FASTCGI_QT_MEMCACHED_CACHE_H
#define _FASTCGI_QT_MEMCACHED_CACHE_H

#include "CacheBackend.h"

#include <QReadWriteLock>

struct memcached_st;

namespace FastCgiQt
{
	/** Cache backend implemented using memcached.
	 *
	 * Binary key format:
	 * 	UTF8 string
	 * Binary data format:
	 * 	little-endian 64-bit UTC timestamp, data
	 */
	class MemcachedCache : public CacheBackend
	{
		public:
			MemcachedCache(const QString& cacheName);
			~MemcachedCache();
			virtual void remove(const QString& key);
			virtual CacheEntry value(const QString& key) const;
			virtual void setValue(const QString& key, const CacheEntry& entry);
			virtual QReadWriteLock* readWriteLock() const;
		private:
			memcached_st* m_memcached;
			const QString m_keyPrefix;
			static QReadWriteLock m_lock;
	};

	class MemcachedCacheFactory : public QObject, public CacheBackend::Factory
	{
		Q_OBJECT
		Q_INTERFACES(FastCgiQt::CacheBackend::Factory)
		public:
			virtual bool loadSettings();
			virtual CacheBackend* getCacheBackend(const QString& cacheName) const;
	};
}

#endif
