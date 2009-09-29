/* LICENSE NOTICE
	Copyright (c) 2009, Frederick Emmott <mail@fredemmott.co.uk>

	Permission to use, copy, modify, and/or distribute this software for any
	purpose with or without fee is hereby granted, provided that the above
	copyright notice and this permission notice appear in all copies.

	THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
	WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
	MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
	ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
	WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
	ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
	OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
#include "Request.h"
#include "Request_Private.h"

#include <QDebug>
#include <QNetworkCookie>
#include <QRegExp>
#include <QUrl>

namespace FastCgiQt
{
	Request::Request(Private* _d, QObject* parent)
	: QIODevice(parent)
	, d(_d)
	{
		open(QIODevice::ReadWrite);
		connect(
			d->device,
			SIGNAL(readyRead()),
			this,
			SIGNAL(readyRead())
		);
	}

	qint64 Request::readData(char* data, qint64 maxSize)
	{
		Q_ASSERT(d->postDataMode == Private::UnknownPostData || d->postDataMode == Private::RawPostData);
		d->postDataMode = Private::RawPostData;
		return d->device->read(data, maxSize);
	}

	qint64 Request::writeData(const char* data, qint64 maxSize)
	{
		QIODevice* device = d->device;

		if(!d->haveSentHeaders)
		{
			d->haveSentHeaders = true;
			for(ClientIODevice::HeaderMap::ConstIterator it = d->responseHeaders.constBegin(); it != d->responseHeaders.constEnd(); ++it)
			{
				device->write(it.key());
				device->write(": ");
				device->write(it.value());
				device->write("\r\n");
			}
			device->write("\r\n");
		}
		return device->write(data, maxSize);
	}

	qint64 Request::size() const
	{
		return QString::fromLatin1(rawValue(ServerData, "CONTENT_LENGTH")).toLongLong();
	}

	QUrl Request::url(UrlPart part) const
	{
		QUrl url;
		// Protocol and host are needed, regardless of part

		///@fixme - HTTPS support
		url.setScheme("http");
		// authority == user:password@host:port - as HTTP_HOST contains user and port, go with that
		url.setAuthority(value(ServerData, "HTTP_HOST"));

		switch(part)
		{
			case RootUrl:
			{
				const int queryStringLength = rawValue(ServerData, "QUERY_STRING").length();
				const int pathInfoLength = rawValue(ServerData, "PATH_INFO").length();
				QByteArray basePath = rawValue(ServerData, "REQUEST_URI");
				basePath.chop(queryStringLength + 1 + pathInfoLength);
				url.setEncodedPath(basePath);
				break;
			}
			case LocationUrl:
			case RequestUrl:
			{
				const int queryStringLength = rawValue(ServerData, "QUERY_STRING").length();
				QByteArray basePath = rawValue(ServerData, "REQUEST_URI");
				basePath.chop(queryStringLength + 1);
				url.setEncodedPath(basePath);
				if(part == RequestUrl)
				{
					url.setEncodedQuery(rawValue(ServerData, "QUERY_STRING"));
				}
				break;
			}
			default:
				qFatal("Unknown URL part: %d", part);
		}
		return url;
	}

	QList<QNetworkCookie> Request::cookies() const
	{
		QList<QNetworkCookie> cookies;
		for(ClientIODevice::HeaderMap::ConstIterator it = d->serverData.constBegin(); it != d->serverData.constEnd(); ++it)
		{
			if(it.key().toUpper() == "HTTP_COOKIE")
			{
				cookies.append(QNetworkCookie::parseCookies(it.value()));
			}
		}
		return cookies;
	}

	void Request::sendCookie(const QNetworkCookie& cookie)
	{
		addHeader("set-cookie", cookie.toRawForm());
	}

	void Request::setHeader(const QByteArray& name, const QByteArray& value)
	{
		Q_ASSERT(!d->haveSentHeaders);
		d->serverData[name] = value;
	}

	void Request::addHeader(const QByteArray& name, const QByteArray& value)
	{
		Q_ASSERT(!d->haveSentHeaders);
		d->serverData.insertMulti(name, value);
	}

	QHash<QByteArray, QByteArray> Request::rawValues(DataSource source) const
	{
		switch(source)
		{
			case GetData:
				return d->getData;
			case PostData:
				d->loadPostVariables();
				return d->postData;
			case ServerData:
				return d->serverData;
			default:
				qFatal("Unknown value type: %d", source);
		}
		return QHash<QByteArray, QByteArray>();
	}

	QByteArray Request::rawValue(DataSource source, const QByteArray& name) const
	{
		return rawValues(source).value(name);
	}

	QString Request::value(DataSource source, const QByteArray& name) const
	{
		return QUrl::fromPercentEncoding(rawValue(source, name));
	}

	Request::~Request()
	{
		delete d;
	}
}
