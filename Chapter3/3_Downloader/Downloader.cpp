/*
 * Copyright (C) 2013-2015 Sergey Kosarevsky (sk@linderdaum.com)
 * Copyright (C) 2013-2015 Viktor Latypov (vl@linderdaum.com)
 * Based on Linderdaum Engine http://www.linderdaum.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must display the names 'Sergey Kosarevsky' and
 *    'Viktor Latypov'in the credits of the application, if such credits exist.
 *    The authors of this work must be notified via email (sk@linderdaum.com) in
 *    this case of redistribution.
 *
 * 3. Neither the name of copyright holders nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "Downloader.h"

#include "Blob.h"
#include "WorkerThread.h"
#include "Event.h"

clDownloader::clDownloader( const clPtr<clAsyncQueue>& Queue )
	: FDownloadThread( make_intrusive<clWorkerThread>() )
	, FEventQueue( Queue )
{
	FDownloadThread->Start();
	FDownloadThread->SetPriority( iThread::Priority_Normal );
}

clDownloader::~clDownloader()
{
	FDownloadThread->CancelAll();
	FDownloadThread->Exit( true );
}

clPtr<clDownloadTask> clDownloader::DownloadURL( const std::string& URL, size_t TaskID, const clPtr<clDownloadCompleteCallback>& CB )
{
	if ( !TaskID || !CB ) { return clPtr<clDownloadTask>(); }

	auto Task = make_intrusive<clDownloadTask>( URL, TaskID, CB, this );

	FDownloadThread->AddTask( Task );

	return Task;
}

void clDownloader::CancelAll()
{
	FDownloadThread->CancelAll();
}

bool clDownloader::CancelLoad( size_t TaskID )
{
	return FDownloadThread->CancelTask( TaskID );
}

size_t clDownloader::GetNumDownloads() const
{
	return FDownloadThread->GetQueueSize();
}

class clCallbackWrapper: public iAsyncCapsule
{
public:
	explicit clCallbackWrapper( const clPtr<clDownloadTask> T ): FTask( T ) {}

	virtual void Invoke() override
	{
		FTask->InvokeCallback();
	}

private:
	clPtr<clDownloadTask> FTask;
};

void clDownloader::CompleteTask( clPtr<clDownloadTask> Task )
{
	if ( !Task->IsPendingExit() )
	{
		if ( FEventQueue )
		{
			FEventQueue->EnqueueCapsule( make_intrusive<clCallbackWrapper>( Task ) );
		}
	}
}
