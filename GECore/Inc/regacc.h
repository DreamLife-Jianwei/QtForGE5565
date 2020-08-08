#ifndef	_REGACC_H_
#define	_REGACC_H_

#include <winreg.h>

#define	DEFAULT_HKEY	HKEY_LOCAL_MACHINE
#define	DEFAULT_SKEY	"SYSTEM"

LONG	LogMsg(DWORD Error, LONG Line, const char *pFormat, ...);

/***************************************************************************
*	Structure used to prevent some values from being removed from a key.
***************************************************************************/
typedef	struct _VALINFO
{
	struct	_VALINFO	*pKeyVals;
			char		*pName;
			char		*pDefVal;
} VALINFO;

/***************************************************************************
*	This class is used to maintain and set a window's position
***************************************************************************/
class	CRegAccess
{
public:
	CRegAccess(	LONG	bProcess,
				HKEY	hKey = DEFAULT_HKEY,
				char	*pSubKey = DEFAULT_SKEY	)
		:	m_hKey(NULL),
			m_hDefKey(hKey),
			m_pDacl(NULL),
			m_bProcess(bProcess)
	{
		/* Grab the subkey name */
		strncpy(m_SubKey, pSubKey, sizeof(m_SubKey));
		m_SubKey[sizeof(m_SubKey) - 1] = 0;
		m_OpenSubKey[0] = 0;
	}
	~CRegAccess()
	{
		/* If we allocated a security DACL */
		if (m_pDacl != NULL)
		{
			/* Free that guy */
			HeapFree(GetProcessHeap(), 0, m_pDacl);
			m_pDacl = NULL;
		}

		/* Be sure the key is closed */
		CloseKey();
	}

	/*************************************************************************
	*	This function is called to open the registry key
	*************************************************************************/
	DWORD	OpenKey(	char	*pSubKey = NULL,
						HKEY	*phKey = NULL,
						REGSAM	Access = KEY_READ	)
	{
		DWORD	Error;
		char	SubKey[256];

		/* If there is no return key specified, used the default */
		if (phKey == NULL)
			phKey = &m_hKey;

		/* Build the subkey */
		strcpy(SubKey, m_SubKey);

		if (pSubKey != NULL)
		{
			strcat(SubKey, "\\");
			strcat(SubKey, pSubKey);
		}

		/* Just try and open it */
		if ((Error = ::RegOpenKeyEx(	m_hDefKey,
										SubKey,
										0,
										Access,
										phKey	)) == ERROR_SUCCESS)
		{
			/* Keep track of the open subkey */
			strcpy(m_OpenSubKey, SubKey);
		}
		return(Error);
	}

	/*************************************************************************
	*	This function is called to close the registry key
	*************************************************************************/
	void	CloseKey()
	{
		/* If the key is there */
		if (m_hKey != NULL)
		{
			/* Close that guy */
			::RegCloseKey(m_hKey);
			m_hKey = NULL;
		}

		/* Kill the open subkey */
		m_OpenSubKey[0] = 0;
	}

	/*************************************************************************
	*	This function is called to locate and open the specified key
	*************************************************************************/
	DWORD	LocateKey(	LONG	&Index,
						char	*pKeyNm,
						char	*pSubKey = NULL	)
	{
		char	SubKey[256];
		char	NewKey[256];
		DWORD	NmSize;
		DWORD	Error;
		HKEY	hParKey = NULL;

		do
		{
			/* Open the subkey */
			if ((Error = OpenKey(pSubKey, &hParKey)) != ERROR_SUCCESS)
				break;

			/* If we got that guy, enumerate the keys under that */
			NmSize = sizeof(SubKey);
			while ((Error = ::RegEnumKeyEx(	hParKey,
											Index,
											SubKey,
											&NmSize,
											NULL,
											NULL,
											NULL,
											NULL	)) == ERROR_SUCCESS)
			{
				/* Get ready for next enumeration */
				Index++;
				NmSize = sizeof(SubKey);

				/* See if this guy matches our string */
				if (FormatMatch(pKeyNm, SubKey) == 0)
				{
					/* Go to the next one */
					continue;
				}

				/* Close the current key */
				CloseKey();

				/* Build the new key */
				if (pSubKey != NULL)
				{
					strcpy(NewKey, pSubKey);
					strcat(NewKey, "\\");
					strcat(NewKey, SubKey);
				}
				else
					strcpy(NewKey, SubKey);

				/* Open that key */
				Error = OpenKey(NewKey);
				break;
			}
		}
		while (0);

		/* If we opened a subkey, close it */
		if (hParKey != NULL)
			::RegCloseKey(hParKey);

		return(Error);
	}

	/*************************************************************************
	*	This function is called to return the subkey that
	*	corresponds to the index specified
	*************************************************************************/
	DWORD	GetKey(	LONG		Index,
					CRegAccess	*pRetKey	)
	{
		char	SubKey[256];
		DWORD	NmSize;
		DWORD	Error;

		do
		{
			/* Use the enumerator to get to the subkey */
			NmSize = sizeof(SubKey);
			if ((Error = ::RegEnumKeyEx(	m_hKey,
											Index,
											SubKey,
											&NmSize,
											NULL,
											NULL,
											NULL,
											NULL	)) != ERROR_SUCCESS)
			{
				/* Blow it off */
				break;
			}

			/* If it's there, we need to update the return key */
			pRetKey->CloseKey();
			pRetKey->m_hDefKey = m_hDefKey;

			/* Set up the subkey name */
			strcpy(pRetKey->m_SubKey, m_OpenSubKey);
			strcat(pRetKey->m_SubKey, "\\");
			strcat(pRetKey->m_SubKey, SubKey);

			/* Open that key */
			Error = pRetKey->OpenKey();
		}
		while (0);

		return(Error);
	}

	/***********************************************************************
	*	Remove a value from an open key
	***********************************************************************/
	DWORD	RemoveVal(	char	*pValue,
						char	*pPad = "",
						HKEY	hKey = NULL	)
	{
		LONG				bOpened = FALSE;
		SECURITY_DESCRIPTOR	*pDesc = NULL;
		DWORD				Error = ERROR_SUCCESS;

		do
		{
			/* See if we're processing */
			if (m_bProcess == FALSE)
			{
				/* Just return an error */
				Error = -1;
				break;
			}

			/* If there a key specifed */
			if (hKey != NULL)
			{
				/* Just delete the value */
				Error = RegDeleteValue(hKey, pValue);
				break;
			}

			do
			{
				/* Get the security descriptor for that key */
				if ((pDesc = GetKeySecurity(m_hDefKey, m_SubKey)) == NULL)
					break;

				/* Change the key's security settings */
				if ((Error = ChangeKeySecurity(	m_hDefKey,
												m_SubKey	)) != ERROR_SUCCESS)
				{
					break;
				}

				/* Open the key */
				if ((Error = ::RegOpenKeyEx(	m_hDefKey,
												m_SubKey,
												0,
												KEY_ALL_ACCESS,
												&hKey	)) != ERROR_SUCCESS)
				{
					/* Uh-oh */
					break;
				}

				/* Just delete the value */
				Error = RegDeleteValue(hKey, pValue);
			}
			while (0);

			/* If the key was opened, close it */
			if (hKey != NULL)
				RegCloseKey(hKey);

			/* If we got a security descriptor, put it back */
			ResetKeySecurity(m_hKey, m_SubKey, pDesc);
		}
		while (0);

		/* Log the message */
		LogMsg(Error, __LINE__, "%sDeleting value %s\n", pPad, pValue);

		return(Error);
	}

	/***********************************************************************
	*	Remove the currently open key
	***********************************************************************/
	DWORD	RemoveKey(	HKEY	hParKey = NULL,
						char	*pSubKey = NULL,
						LONG	Level = 0	)
	{
		LONG	Index;
		DWORD	NmSize;
		char	Pad[256];
		char	SubKey[256];
		char	KeyNm[256];
		HKEY	hKey = NULL;
		DWORD	Error = ERROR_SUCCESS;

		do
		{
			/* If this is the top level */
			if (pSubKey == NULL)
			{
				/* Get the open subkey */
				strcpy(KeyNm, m_OpenSubKey);
				pSubKey = &KeyNm[0];

				/* Get the parent key */
				hParKey = m_hDefKey;

				/* Close the open key */
				CloseKey();
			}

			/* Make sure we're going to process this call */
			if (m_bProcess == TRUE)
			{
				/* Change the key's security settings */
				if ((Error = ChangeKeySecurity(	hParKey,
												pSubKey	)) != ERROR_SUCCESS)
				{
					/* Uh-oh */
					break;
				}
			}

			/* Open the key */
			if ((Error = ::RegOpenKeyEx(	hParKey,
											pSubKey,
											0,
											KEY_ALL_ACCESS,
											&hKey	)) != ERROR_SUCCESS)
			{
				/* Uh-oh */
				break;
			}

			/* Go through through the subkeys */
			Index = 0;
			for (;;)
			{
				/* Get the key */
				NmSize = sizeof(SubKey);
				if ((Error = ::RegEnumKeyEx(	hKey,
												Index,
												SubKey,
												&NmSize,
												NULL,
												NULL,
												NULL,
												NULL	)) != ERROR_SUCCESS)
				{
					/* Blow it off */
					break;
				}

				/* Recursively remove that key */
				if (RemoveKey(hKey, SubKey, Level + 1) != ERROR_SUCCESS)
				{
					/* Goto the next one */
					Index++;
				}
			}

			/* Make sure we're going to process this call */
			if (m_bProcess == FALSE)
			{
				/* If not, just log it and return a bad status */
				Error = -1;
				break;
			}

			/* Delete this key */
			Error = RegDeleteKey(hParKey, pSubKey);
		}
		while (0);

		/* Build the pad buffer */
		Pad[0] = 0;
		while (Level-- != 0)
			strcat(Pad, "  ");

		/* Log that message */
		if (Error != ERROR_FILE_NOT_FOUND)
		{
			LogMsg(	Error, __LINE__, "%sRemoving key %s\\%s\n",
								Pad, GetMainKey(), pSubKey);
		}

		/* If the key was opened, close it */
		if (hKey != NULL)
			RegCloseKey(hKey);

		return(Error);
	}

	/***********************************************************************
	*	Enumerate the open key and remove all values and keys
	*	that do not exist in the supplied pInfo list.
	***********************************************************************/
	DWORD	RemoveAllExcept(	VALINFO	*pValInfo,
								HKEY	hParKey = NULL,
								char	*pSubKey = NULL,
								LONG	Level = 0	)
	{
		LONG				Index;
		DWORD				NmSize;
		char				SubKey[256];
		char				KeyNm[256];
		char				Pad[256];
		VALINFO				*pInfo;
		SECURITY_DESCRIPTOR	*pDesc = NULL;
		HKEY				hKey = NULL;
		DWORD				Error = ERROR_SUCCESS;

		do
		{
			/* Build the pad buffer */
			Pad[0] = 0;
			for (Index = 0; Index < Level; Index++)
				strcat(Pad, "  ");

			/* If this is the top level */
			if (pSubKey == NULL)
			{
				/* Get the open subkey */
				strcpy(KeyNm, m_OpenSubKey);
				pSubKey = &KeyNm[0];

				/* Get the parent key */
				hParKey = m_hDefKey;
			}

			/* Get the security descriptor for that key */
			if ((pDesc = GetKeySecurity(hParKey, pSubKey)) == NULL)
				break;

			/* Change the key's security settings */
			if ((Error = ChangeKeySecurity(hParKey, pSubKey)) != ERROR_SUCCESS)
				break;

			/* Open the key */
			if ((Error = ::RegOpenKeyEx(	hParKey,
											pSubKey,
											0,
											KEY_ALL_ACCESS,
											&hKey	)) != ERROR_SUCCESS)
			{
				/* Uh-oh */
				break;
			}

			/* Go through through the subkeys */
			Index = 0;
			for (;;)
			{
				/* Get the key */
				NmSize = sizeof(SubKey);
				if ((Error = ::RegEnumKeyEx(	hKey,
												Index,
												SubKey,
												&NmSize,
												NULL,
												NULL,
												NULL,
												NULL	)) != ERROR_SUCCESS)
				{
					/* Blow it off */
					break;
				}

				/* See if this guy is in the list */
				pInfo = pValInfo;
				while (pInfo->pName != NULL)
				{
					/* We're only looking for subdirs, so check that */
					if (pInfo->pKeyVals == NULL)
					{
						/* No match, goto the next one */
						pInfo++;
						continue;
					}

					/* See if this name matches the sub key name */
					if (stricmp(pInfo->pName, SubKey) == 0)
					{
						/* If it does match, process that subkey */
						RemoveAllExcept(	pInfo->pKeyVals,
											hKey,
											SubKey,
											Level + 1	);

						/* We're done with that one */
						break;
					}

					/* No match, goto the next one */
					pInfo++;
				}

				/* If we did not find this guy in the list */
				if (pInfo->pName == NULL)
				{
					if (m_bProcess == FALSE)
					{
						/* Just log the message */
						LogMsg(-1, __LINE__,"%sDeleting key %s\n", Pad, SubKey);
						break;
					}
					else
					{
						/* Make sure we can delete that guy */
						ChangeKeySecurity(hKey, SubKey);

						/* We need to delete this key */
						if ((Error = RegDeleteKey(	hKey,
													SubKey )) != ERROR_SUCCESS)
						{
							/* If it fails, goto the next one */
							Index++;
						}

						/* Log the message */
						LogMsg(	Error, __LINE__,
									"%sDeleting key %s\n", Pad, SubKey);
					}
				}
				else
				{
					/* Otherwise, it was already processed, goto the next key */
					Index++;
				}
			}

			/* Go through the values of this subkey */
			Index = 0;
			for (;;)
			{
				/* Get the key */
				NmSize = sizeof(SubKey);
				if ((Error = ::RegEnumValue(	hKey,
												Index,
												SubKey,
												&NmSize,
												NULL,
												NULL,
												NULL,
												NULL	)) != ERROR_SUCCESS)
				{
					/* Blow it off */
					break;
				}

				/* See if this guy is in the list */
				pInfo = pValInfo;
				while (pInfo->pName != NULL)
				{
					/* We're only looking for values, so check that */
					if (pInfo->pKeyVals != NULL)
					{
						/* No match, goto the next one */
						pInfo++;
						continue;
					}

					/* See if this name matches the sub key name */
					if (stricmp(pInfo->pName, SubKey) == 0)
					{
						DWORD	DataSize;

						/* Found one, see if we need to change his value */
						if (pInfo->pDefVal == NULL)
							break;

						if (m_bProcess == FALSE)
						{
							LogMsg(	-1, __LINE__,
									"%sResetting value %s to %s\n",
									Pad, SubKey, pInfo->pDefVal	);
							break;
						}
	
						/* Reset that value */
						DataSize = strlen(pInfo->pDefVal) + 1;
						Error = ::RegSetValueEx(	hKey,
													pInfo->pName,
													0,
													REG_SZ,
													(BYTE *)(pInfo->pDefVal),
													DataSize	);

						LogMsg(	Error, __LINE__,
								"%sResetting value %s to %s\n",
								Pad, SubKey, pInfo->pDefVal	);

						break;
					}

					/* Try the next string */
					pInfo++;
				}

				/* If we did not find this guy in the list */
				if (pInfo->pName == NULL)
				{
					/* We need to delete this value */
					if (RemoveVal(SubKey, Pad, hKey) != ERROR_SUCCESS)
					{
						/* If it fails, goto the next one */
						Index++;
					}
				}
				else
				{
					/* Otherwise, it was already processed, goto the next val */
					Index++;
				}
			}

		}
		while (0);

		/* If the key is open, close it */
		if (hKey != NULL)
			RegCloseKey(hKey);

		/* If we got a security descriptor, put it back */
		ResetKeySecurity(hParKey, pSubKey, pDesc);

		return(Error);
	}

	/***********************************************************************
	*	Get a value from the registry, given it's name
	***********************************************************************/
	DWORD	GetVal(	char	*pName,
					void	*pData,
					DWORD	DataSize,
					char	*pSubKey = NULL	)
	{
		DWORD	Error = ERROR_INVALID_PARAMETER;
		BOOL	bOpened = FALSE;

		/* Make sure the key is open */
		if (m_hKey == NULL)
		{
			/* If not, try to open it */
			OpenKey(pSubKey);
			bOpened = TRUE;
		}

		/* If the key is there */
		if (m_hKey != NULL)
		{
			/* Go get the info from the registry */
			Error = ::RegQueryValueEx(	m_hKey,
										pName,
										NULL,
										NULL,
										(BYTE *)pData,
										&DataSize	);
		}

		/* If we opened the key, close it */
		if (bOpened == TRUE)
			CloseKey();

		return(Error);
	}

	/***********************************************************************
	*	Get a value from the registry given it's index
	***********************************************************************/
	DWORD	GetVal(	LONG	Index,
					void	*pData,
					DWORD	DataSize	)
	{
		DWORD	Error = ERROR_INVALID_PARAMETER;
		BOOL	bOpened = FALSE;

		/* Make sure the key is open */
		if (m_hKey == NULL)
		{
			/* If not, try to open it */
			OpenKey();
			bOpened = TRUE;
		}

		/* If the key is there */
		if (m_hKey != NULL)
		{
			DWORD	Size;

			/* Get the key */
			Error = ::RegEnumValue(	m_hKey,
									Index,
									(char *)pData,
									&DataSize,
									NULL,
									NULL,
									NULL,
									&Size	);
		}

		/* If we opened the key, close it */
		if (bOpened == TRUE)
			CloseKey();

		return(Error);
	}

	/***********************************************************************
	*	Set a binary value in the registry
	***********************************************************************/
	DWORD	SetVal(	char	*pName,
					void	*pData,
					DWORD	DataSize,
					char	*pSubKey = NULL	)
	{
		DWORD	Error = ERROR_INVALID_PARAMETER;
		BOOL	bOpened = FALSE;

		/* Make sure the key is open */
		if (m_hKey == NULL)
		{
			/* If not, try to open it */
			OpenKey(pSubKey, NULL, KEY_SET_VALUE);
			bOpened = TRUE;
		}

		/* If the key is there */
		if (m_hKey != NULL)
		{
			/* Stuff that info into the registry */
			Error = ::RegSetValueEx(	m_hKey,
										pName,
										0,
										REG_BINARY,
										(BYTE *)pData,
										DataSize	);
		}

		/* If we opened the key, close it */
		if (bOpened == TRUE)
			CloseKey();

		return(Error);
	}

	/***********************************************************************
	*	Set a string value in the registry
	***********************************************************************/
	DWORD	SetVal(	char	*pName,
					char	*pData,
					char	*pSubKey = NULL	)
	{
		DWORD	DataSize;
		BOOL	bOpened = FALSE;
		DWORD	Error = ERROR_INVALID_PARAMETER;

		/* Make sure the key is open */
		if (m_hKey == NULL)
		{
			/* If not, try to open it */
			OpenKey(pSubKey, NULL, KEY_SET_VALUE);
			bOpened = TRUE;
		}

		/* If the key is there */
		if (m_hKey != NULL)
		{
			/* Stuff that info into the registry */
			DataSize = strlen(pData) + 1;
			Error = ::RegSetValueEx(	m_hKey,
										pName,
										0,
										REG_EXPAND_SZ,
										(BYTE *)pData,
										DataSize	);
		}

		/* If we opened the key, close it */
		if (bOpened == TRUE)
			CloseKey();

		return(Error);
	}

	/***********************************************************************
	*	Set a DWORD value in the registry
	***********************************************************************/
	DWORD	SetVal(	char	*pName,
					DWORD	Data,
					char	*pSubKey = NULL	)
	{
		DWORD	DataSize;
		BOOL	bOpened = FALSE;
		DWORD	Error = ERROR_INVALID_PARAMETER;

		/* Make sure the key is open */
		if (m_hKey == NULL)
		{
			/* If not, try to open it */
			OpenKey(pSubKey, NULL, KEY_SET_VALUE);
			bOpened = TRUE;
		}

		/* If the key is there */
		if (m_hKey != NULL)
		{
			/* Stuff that info into the registry */
			DataSize = sizeof(DWORD);
			Error = ::RegSetValueEx(	m_hKey,
										pName,
										0,
										REG_DWORD,
										(BYTE *)&Data,
										DataSize	);
		}

		/* If we opened the key, close it */
		if (bOpened == TRUE)
			CloseKey();

		return(Error);
	}

	/************************************************************************
	*	Called to copy the current key to the specified destination key
	************************************************************************/
	DWORD	CopyKey(	HKEY	hDstPar,
						char	*pDstSub,
						HKEY	hSrcPar = NULL	)
	{
		LONG	Index;
		DWORD	NmSize;
		char	SubKey[256];
		HKEY	hDstKey = NULL;
		DWORD	Error = ERROR_SUCCESS;

		do
		{
			/* If the source of the parent tree is not specified */
			if (hSrcPar == NULL)
			{
				/* Use the open key */
				hSrcPar = m_hKey;
			}

			/* Create the destination key */
			if ((Error = RegCreateKeyEx(	hDstPar,
											pDstSub,
											0,
											NULL,
											REG_OPTION_NON_VOLATILE,
											KEY_ALL_ACCESS,
											NULL,
											&hDstKey,
											NULL	)) != ERROR_SUCCESS)
			{
				/* Bad news */
				break;
			}

			/* Go through the subkeys of the source key */
			Index = 0;
			for (;;)
			{
				HKEY	hSrcKey = NULL;

				/* Get the key */
				NmSize = sizeof(SubKey);
				if ((Error = ::RegEnumKeyEx(	hSrcPar,
												Index,
												SubKey,
												&NmSize,
												NULL,
												NULL,
												NULL,
												NULL	)) != ERROR_SUCCESS)
				{
					/* Blow it off */
					break;
				}

				/* Open that source key */
				if (::RegOpenKeyEx(	hSrcPar,
									SubKey,
									0,
									KEY_READ,
									&hSrcKey	) == ERROR_SUCCESS)
				{
					/* Recursively copy that key */
					CopyKey(hDstKey, SubKey, hSrcKey);

					/* Close the source key */
					RegCloseKey(hSrcKey);
					hSrcKey = NULL;
				}

				/* Goto the next one */
				Index++;
			}

			/* Go through the values of the source key */
			Index = 0;
			for (;;)
			{
				DWORD	Type;
				DWORD	Size;
				BYTE	*pData;

				/* Get the key */
				NmSize = sizeof(SubKey);
				if ((Error = ::RegEnumValue(	hSrcPar,
												Index,
												SubKey,
												&NmSize,
												NULL,
												NULL,
												NULL,
												&Size	)) != ERROR_SUCCESS)
				{
					/* Blow it off */
					break;
				}

				/* Allocate a blob for the data */
				if ((pData = (BYTE *)(calloc(Size, 1))) != NULL)
				{
					/* Make the call again to get the data */
					NmSize = sizeof(SubKey);
					if (::RegEnumValue(	hSrcPar,
										Index,
										SubKey,
										&NmSize,
										NULL,
										&Type,
										pData,
										&Size	) == ERROR_SUCCESS)
					{
						/* Write the value */
						::RegSetValueEx(	hDstKey,
											SubKey,
											0,
											Type,
											pData,
											Size	);
					}

					/* Free the data */
					free(pData);
				}

				/* Goto the next one */
				Index++;
			}

		}
		while (0);

		/* If the destination key was created, close the handle */
		if (hDstKey != NULL)
			RegCloseKey(hDstKey);

		return(Error);
	}

	/************************************************************************
	*	Called to see if the current location matches the format
	************************************************************************/
	LONG	FormatMatch(	char	*pFmt,
							char	*pInp	)
	{
		LONG	FmtLen = 0;

		/* Go through the format string */
		while ((*pFmt != 0) && (*pInp != 0))
		{
			/* See if this is a format character */
			if (*pFmt == '%')
			{
				/* Check out the next character */
				pFmt++;

				if (*pFmt == 'd')
				{
					/* Make sure the input matches a digit */
					if ((*pInp < '0') || (*pInp > '9'))
						break;

					/* Got a match, move to next */
					pFmt++;
					pInp++;
					FmtLen++;
				}
				else if (*pFmt == 'c')
				{
					/* Could be a digit or letter */
					if ((*pInp >= '0') && (*pInp <= '9'))
					{
						/* Got a match, move to next */
						pFmt++;
						pInp++;
						FmtLen++;
					}
					else if ((*pInp >= 'a') && (*pInp <= 'z'))
					{
						/* Got a match, move to next */
						pFmt++;
						pInp++;
						FmtLen++;
					}
					else if ((*pInp >= 'A') && (*pInp <= 'Z'))
					{
						/* Got a match, move to next */
						pFmt++;
						pInp++;
						FmtLen++;
					}
					else
						break;
				}
				else
				{
					/* No match */
					break;
				}
			}
			else if (*pFmt == '?')
			{
				/* This guy is always a match */
				pFmt++;
				pInp++;
				FmtLen++;
			}
			else
			{
				/* It's gotta match the input string */
				if ((*pFmt | 0x20) != (*pInp | 0x20))
					break;

				/* Go to the next one */
				pFmt++;
				pInp++;
				FmtLen++;
			}
		}

		/* If we didn't make it to the end of the format string */
		if (*pFmt != 0)
		{
			/* No match */
			FmtLen = 0;
		}
		return(FmtLen);
	}

protected:
	/************************************************************************
	*	Called to obtain the string that matches the main key
	************************************************************************/
	char	*GetMainKey(HKEY hKey = NULL)
	{
		char	*pName = NULL;

		if (hKey == NULL)
			hKey = m_hDefKey;

		if (hKey ==	HKEY_CLASSES_ROOT)
		{
			pName = "HKCR";
		}
		else if (hKey == HKEY_CURRENT_CONFIG)
		{
			pName = "HKCC";
		}
		else if (hKey == HKEY_CURRENT_USER)
		{
			pName = "HKCU";
		}
		else if (hKey == HKEY_LOCAL_MACHINE)
		{
			pName = "HKLM";
		}
		else if (hKey == HKEY_USERS)
		{
			pName = "HKU";
		}
		else
		{
			pName = "User Key";
		}
		return(pName);
	}

	/************************************************************************
	*	Called to change the permissions on a key so that it can be removed
	************************************************************************/
	DWORD	ChangeKeySecurity(	HKEY	hParKey,
								char	*pKey	)
	{
		PSID	pSid = NULL;
		HKEY	hKey = NULL;
		DWORD	Error = ERROR_SUCCESS;

		do
		{
			/* See if we have already allocated the DACL */
			if (m_pDacl == NULL)
			{
				DWORD						AclSize;
				SID_IDENTIFIER_AUTHORITY	Sia = SECURITY_NT_AUTHORITY;

				/* Prepare Sid to represent any interactively logged-on user */
				if (AllocateAndInitializeSid(	&Sia,
												1,
												SECURITY_INTERACTIVE_RID,
												0, 0, 0, 0, 0, 0, 0,
												&pSid	) == FALSE)
				{
					/* Get the error */
					Error = GetLastError();
					break;
				}

				/* Compute size of new acl */
				AclSize = sizeof(ACL);
				AclSize += (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD));
				AclSize += GetLengthSid(pSid);

				/* Allocate storage for Acl */
				if ((m_pDacl = (PACL)HeapAlloc(	GetProcessHeap(),
												0,
												AclSize	)) == NULL)
				{
					/* Uh-oh */
					Error = ERROR_OUTOFMEMORY;
					break;
				}

				/* Initialize the ACL */
				if (InitializeAcl(m_pDacl, AclSize, ACL_REVISION) == FALSE)
				{
					/* Get the error */
					Error = GetLastError();
					break;
				}

				/* Grant the Interactive Sid KEY_ALL_ACCESS access to the key */
				if (AddAccessAllowedAce(	m_pDacl,
											ACL_REVISION,
											KEY_ALL_ACCESS,
											pSid	) == FALSE)
				{	
					/* Get the error */
					Error = GetLastError();
					break;
				}

				/* Initialize the descriptor */
				if (InitializeSecurityDescriptor(
										&m_Sd,
										SECURITY_DESCRIPTOR_REVISION) == FALSE)
				{
					/* Get the error */
					Error = GetLastError();
					break;
				}

				/* Attach that to the DACL */
				if (SetSecurityDescriptorDacl(	&m_Sd,
												TRUE,
												m_pDacl,
												FALSE	) == FALSE)
				{
					/* Get the error */
					Error = GetLastError();
					break;
				}
			}

			/* Open the key */
			if ((Error = RegOpenKeyEx(	hParKey,
										pKey,
										0,
										WRITE_DAC,
										&hKey	)) != ERROR_SUCCESS)
			{
				break;
			}

			/* Apply the security descriptor to the registry key */
			if ((Error = RegSetKeySecurity(
							hKey,
							(SECURITY_INFORMATION)DACL_SECURITY_INFORMATION,
							&m_Sd	)) != ERROR_SUCCESS)
			{
				/* Uh-oh */
				break;
			}
		}
		while (0);

		/* Free the Sid */
		if (pSid != NULL)
			FreeSid(pSid);

		/* If the main key was opened, close it */
		if (hKey != NULL)
			RegCloseKey(hKey);

		return(Error);
	}

	/************************************************************************
	*	Called to obtain a key's security descriptor
	*	Return value must be freed with HeapFree if valid
	************************************************************************/
	SECURITY_DESCRIPTOR	*GetKeySecurity(	HKEY	hParKey,
											char	*pKey	)
	{
		DWORD					Size;
		SECURITY_INFORMATION	SecInfo;
		SECURITY_DESCRIPTOR		*pDesc = NULL;
		HKEY					hKey = NULL;

		do
		{
			/* Open that key */
			if (RegOpenKeyEx(	hParKey,
								pKey,
								0,
								KEY_READ,
								&hKey	) != ERROR_SUCCESS)
			{
				break;
			}

			/* Set up the info we'll want */
			SecInfo = (SECURITY_INFORMATION)DACL_SECURITY_INFORMATION;

			/* Now find out how large the data is going to be */
			Size = 0;
			if (RegGetKeySecurity(	hKey,
									SecInfo,
									pDesc,
									&Size	) != ERROR_INSUFFICIENT_BUFFER)
			{
				/* Uh-oh */
				break;
			}

			/* Allocate the memory for that guy */
			if ((pDesc = (SECURITY_DESCRIPTOR *)HeapAlloc(	GetProcessHeap(),
															0,
															Size	)) == NULL)
			{
				/* Uh-oh */
				break;
			}

			/* Now get the descriptor */
			if (RegGetKeySecurity(	hKey,
									SecInfo,
									pDesc,
									&Size	) != ERROR_SUCCESS)
			{
				/* Uh-oh */
				break;
			}

			/* Verify that pointer */
			if (IsValidSecurityDescriptor(pDesc) == FALSE)
			{
				HeapFree(GetProcessHeap(), 0, pDesc);
				pDesc = NULL;
				break;
			}
		}
		while (0);

		/* If the main key was opened, close it */
		if (hKey != NULL)
			RegCloseKey(hKey);

		/* Return the descriptor */
		return(pDesc);
	}

	/************************************************************************
	*	Called to reset a key's security to that returned by
	*	GetKeySecurity.  The descriptor is also freed.
	************************************************************************/
	DWORD	ResetKeySecurity(	HKEY				hParKey,
								char				*pKey,
								SECURITY_DESCRIPTOR	*pDesc)
	{
		SECURITY_INFORMATION	SecInfo;
		HKEY					hKey = NULL;
		DWORD					Error = ERROR_SUCCESS;

		do
		{
			/* Verify the descriptor */
			if (IsValidSecurityDescriptor(pDesc) == FALSE)
				break;

			/* Open that key */
			if (RegOpenKeyEx(	hParKey,
								pKey,
								0,
								WRITE_DAC,
								&hKey	) != ERROR_SUCCESS)
			{
				break;
			}

			/* Set up the info we'll want */
			SecInfo = (SECURITY_INFORMATION)DACL_SECURITY_INFORMATION;

			/* Apply the security descriptor to the registry key */
			if ((Error = RegSetKeySecurity(	hKey,
											SecInfo,
											pDesc	)) != ERROR_SUCCESS)
			{
				/* Uh-oh */
				break;
			}
		}
		while (0);

		/* If there is a descriptor, free it */
		if (pDesc != NULL)
			HeapFree(GetProcessHeap(), 0, pDesc);

		/* If the main key was opened, close it */
		if (hKey != NULL)
			RegCloseKey(hKey);

		return(Error);
	}

public:
	HKEY					m_hKey;
	HKEY					m_hDefKey;
	LONG					m_bProcess;
	char					m_SubKey[256];
	char					m_OpenSubKey[256];
	PACL					m_pDacl;
	SECURITY_DESCRIPTOR		m_Sd;
};

#endif
