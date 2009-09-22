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
#pragma once

#include "Responder.h"

#include <event.h>
#include <evhttp.h>

#include <QThread>

namespace FastCgiQt
{
	class HttpDaemon : public QThread
	{
		Q_OBJECT
		public:
			HttpDaemon(Responder::Generator responderGenerator, quint16 port, QObject* parent = NULL);
			~HttpDaemon();
		protected:
			void run();
		private:
			/// C callback, just call the member function
			static void handleRequest(struct evhttp_request* request, void* instance);
			void handleRequest(struct evhttp_request* request);

			/// Pointer to function creating new Responder objects.
			Responder::Generator m_responderGenerator;

			/// Port number to run on
			const quint16 m_port;
			/// Handle for libevent base
			struct event_base* m_baseHandle;
			/// Handle for main http object
			struct evhttp* m_httpHandle;
	};
};
