#pragma once

/////////////////////////////////////////////////////////////////////////////
//	CSingleInstance	
//		Limits an application to a single instance
	
class CSingleInstance
{
//-------------private functions----------------------------------------
private:
// Private Construction
	CSingleInstance();
	~CSingleInstance();

//-------------public functions-----------------------------------------
public:
// Accessor
	static CSingleInstance& Instance();
	
// Operations:
// Register causes a mutex to be created
	bool Register();	
	void Unregister();

// Attributes
	void SetInstanceName(PCTSTR pszName);

// Save the current instance HWND in a memory mapped file (optinal)
	void SetInstanceWindow(CWnd *pWnd=NULL)
		{ SetInstanceWindow(pWnd!=NULL ? pWnd->m_hWnd : AfxGetMainWnd()->GetSafeHwnd()); }
	void SetInstanceWindow(HWND hWnd);

// Find previous instance
	HWND FindInstance();

// launch a previous running instance
	bool ActivatePrevInstance();

// The name is free to allow other programs to find the instance.  
// Find previous instance. Note that the name is converted with MakeUpper.
// Because a mutex is case sensitive.
	static HWND FindInstance(PCTSTR pszName);

//-------------protected member functions-------------------------------
protected:
	static CString FormatInternalName(PCTSTR pszName=NULL);
	static void CreateMutex(PCTSTR pszName, HANDLE &hMutex, bool &bCreated);

//-------------data member----------------------------------------------
protected:
	HANDLE	m_hMutexApplication;	// Mutex to lock the Application
									// and to control access to the data
	HANDLE	m_hSharedMemFile;		// memory mapped file to share the data
	CString m_strName;				// Name of the instance (may be file name of the module)
};
