BOOL AsyncDownloadTask( PCHAR Url, LPBYTE *lpBuffer, LPDWORD dwSize ) {
	PCHAR  Host =  NULL;
	PCHAR  Path =  NULL;
	PCHAR  Port =  0;


	PASYNCHTTP  pData =  (PASYNCHTTP)RtlHeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( PASYNCHTTP ) );
	if ( !pData ) {
		return false;
	}

	pData->hConnectedEvent		 =  CreateEventW( NULL, FALSE, FALSE, NULL );
        pData->hRequestOpenedEvent	 =  CreateEventW( NULL, FALSE, FALSE, NULL );
        pData->hRequestCompleteEvent     =  CreateEventW( NULL, FALSE, FALSE, NULL );

	PCHAR  UserAgent  =  (PCHAR)RtlHeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 1024 );
	DWORD  dwUserSize =  1024;
	ObtainUserAgentString( 0, UserAgent, &dwUserSize );

	pData->hInstance = (HINTERNET)InternetOpenA( UserAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, INTERNET_FLAG_ASYNC );

	LPBYTE  lpBuf	  =  NULL;
	DWORD   dwBufSize =  0;

	if ( pData->hInstance ) {
		if ( InternetSetStatusCallback( pData->hInstance, (INTERNET_STATUS_CALLBACK)&Callback) != INTERNET_INVALID_STATUS_CALLBACK) {
			pData->dwCurrent = 1;
			pData->hConnect  = (HINTERNET)InternetConnectA( pData->hInstance, Host, INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, (DWORD_PTR)pData );
			
			if ( !pData->hConnect ) {
				if ( pGetLastError() != ERROR_IO_PENDING ) {
					return false;
				}
				WaitForSingleObject( pData->hConnectedEvent, INFINITE );
			}

			pData->dwCurrent = 2;
			pData->hRequest  = (HINTERNET)HttpOpenRequestA( pData->hConnect, "GET", Path, NULL, NULL, NULL, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, (DWORD_PTR)pData );

			if ( !pData->hRequest ) {
				if ( GetLastError() != ERROR_IO_PENDING ) {
					return false;
				}
				WaitForSingleObject( pData->hRequestOpenedEvent, INFINITE );
			}

			if ( !(BOOL)HttpSendRequestA( pData->hRequest, NULL, 0, NULL, 0 ) ) {
				if ( GetLastError() != ERROR_IO_PENDING ) {
					return false;
				}
			}
			WaitForSingleObject( pData->hRequestCompleteEvent, INFINITE );
			
			LPBYTE pTmpBuf  =  (LPBYTE)RtlHeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 4096 );
			if ( !pTmpBuf ) {
				return false;
			}

			INTERNET_BUFFERSA ib;
			m_memset( &ib, 0, sizeof( INTERNET_BUFFERSA ) );
			ib.dwStructSize   =  sizeof( INTERNET_BUFFERSA );
			ib.lpvBuffer	  =  pTmpBuf;
			
			do {
				ib.dwBufferLength = 4096;

				if ( !(BOOL)InternetReadFileExA( pData->hRequest, &ib, 0, 2 ) ) {
					if ( GetLastError() == ERROR_IO_PENDING) {
						WaitForSingleObject( pData->hRequestCompleteEvent, INFINITE );
					}
					else {
						return false;
					}
				}

				if ( ib.dwBufferLength ) {
					if ( !lpBuf ) {
						if ( !( lpBuf = (LPBYTE)RtlHeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ib.dwBufferLength + 1 ) ) ) {
							return false;
						}
					}
					else {
						LPBYTE p = (LPBYTE)realloc( lpBuf, dwBufSize + ib.dwBufferLength + 1 );

						if ( !p ){
							return false;
						}
						lpBuf = p;
					}

					m_memcpy( lpBuf + dwBufSize, pTmpBuf, ib.dwBufferLength );
					dwBufSize += ib.dwBufferLength;
				}
				else
				{
					pData->IsDownloaded = true;
				}

			} while ( !pData->IsDownloaded );
		}
	}

	InternetCloseHandle( pData->hRequest  );
	InternetCloseHandle( pData->hConnect  );
	InternetCloseHandle( pData->hInstance );

	CloseHandle( pData->hConnectedEvent       );
	CloseHandle( pData->hRequestOpenedEvent   );
	CloseHandle( pData->hRequestCompleteEvent );

	free( pData );
	

	if ( dwSize ) {
		*lpBuffer  =  lpBuf;
		*dwSize    =  dwBufSize;
		return true;
	}

	return false;
}
