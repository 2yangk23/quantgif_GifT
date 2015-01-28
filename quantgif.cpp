// quantgif.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "dllmain.cpp"

#pragma comment (lib, "FreeImage.lib")

#pragma pack(push,gifpacking,1)


typedef struct {
	char cSignature[3]; // Must be 'GIF'
	char cVersion[3];   // Must be '89a'
} GIF_HEADER;

typedef struct { // 7 bytes
	unsigned short iWidth;
	unsigned short iHeight;
	unsigned char iSizeOfGct : 3;
	unsigned char iSortFlag : 1;
	unsigned char iColorResolution : 3;
	unsigned char iGctFlag : 1;
	unsigned char iBackgroundColorIndex;
	unsigned char iPixelAspectRatio;
} GIF_LOGICAL_SCREEN_DESCRIPTOR;

typedef struct { // 6 bytes
	unsigned char iBlockSize;           // Must be 4
	unsigned char iTransparentColorFlag : 1;
	unsigned char iUserInputFlag : 1;
	unsigned char iDisposalMethod : 3;
	unsigned char iReserved : 3;
	unsigned short iDelayTime;
	unsigned char iTransparentColorIndex;
	unsigned char iBlockTerminator;     // Must be 0
} GIF_GRAPHIC_CONTROL_EXTENSION;

typedef struct { // 9 bytes
	unsigned short iLeft;
	unsigned short iTop;
	unsigned short iWidth;
	unsigned short iHeight;
	unsigned char iSizeOfLct : 3;
	unsigned char iReserved : 2;
	unsigned char iSortFlag : 1;
	unsigned char iInterlaceFlag : 1;
	unsigned char iLctFlag : 1;
} GIF_IMAGE_DESCRIPTOR;

#pragma pack(pop,gifpacking)

unsigned short iGctSize[]={6,12,24,48,96,192,384,768};

std::string conv(wchar_t * wstr)
{
    int size = WideCharToMultiByte( CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL );

	char *buffer = new char[ size+1];
	WideCharToMultiByte( CP_UTF8, 0, wstr, -1, buffer, size, NULL, NULL );

	std::string str(buffer);
	delete []buffer;

    return str;
}

bool dirExists(const wchar_t *dirName)
{
    //String conversion
    int size = WideCharToMultiByte(CP_UTF8, 0, dirName, -1, NULL, 0, NULL, NULL );

	char *buffer = new char[ size+1];
	WideCharToMultiByte( CP_UTF8, 0, dirName, -1, buffer, size, NULL, NULL );

	std::string str(buffer);
	delete []buffer;

    DWORD ftyp = GetFileAttributesA(str.c_str());

    if (ftyp == INVALID_FILE_ATTRIBUTES) { //Invalid path
        return false;
    }

    if (ftyp & FILE_ATTRIBUTE_DIRECTORY) { //Valid directory
        return true;
    }

    return false; //Invalid Directory
}

extern "C"
{
    DECLDIR int quantizeGif(wchar_t *path, int quantize, int fps)
    {
        const std::wstring dir = std::wstring(path);
        if(!dirExists(path))
        {
            std::wcout << "The path " << dir << " does not exist" << std::endl;
            return -1;
        }
        std::cout << "Converting files..." << std::endl;

        int delay = 100 / fps;
        FIBITMAP *pic, *bmp, *gif;
        std::wstring iFile, oFile;
        std::wostringstream ws;

        int index = dir.find_last_of('\\');
        std::wstring name = dir.substr(index + 1);

        DWORD dw;
	    char szColorTable[768];
	    unsigned char c;
	    HANDLE hFileOut, hFileIn;
	    GIF_HEADER gh;
	    GIF_LOGICAL_SCREEN_DESCRIPTOR glsd1, glsd;
	    GIF_GRAPHIC_CONTROL_EXTENSION ggce;
	    GIF_IMAGE_DESCRIPTOR gid;
	    ZeroMemory(&glsd1, sizeof(GIF_LOGICAL_SCREEN_DESCRIPTOR));

        ws << dir << "\\" << name << L".gif";
        std::wcout << L"Output file: " << ws.str() << std::endl;
        hFileOut = CreateFile(ws.str().c_str(), GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
        ws.str(std::wstring());
	    strncpy((char *)&gh, "GIF89a", 6);
	    WriteFile(hFileOut, &gh, sizeof(GIF_HEADER), &dw, 0);
	    WriteFile(hFileOut, &glsd1, sizeof(GIF_LOGICAL_SCREEN_DESCRIPTOR), &dw, 0);
	    WriteFile(hFileOut, "\41\377\013NETSCAPE2.0\003\001\377\377\0", 19, &dw, 0);

        for(int i = 1; i < 9999; i++)
        {
            ws << dir << "\\" << i << L".png";
            iFile = ws.str();
            ws.str(std::wstring());

            ws << dir << "\\" << i << L".gif";
            oFile = ws.str();
            ws.str(std::wstring());

            pic = FreeImage_LoadU(FIF_PNG, iFile.c_str());
            if(pic == NULL)
            {
                std::wcout << "Error opening file: " << iFile << std::endl;
                break;
            }
            else
            {
                bmp = FreeImage_ConvertTo24Bits(pic);
                gif = FreeImage_ColorQuantizeEx(bmp, (FREE_IMAGE_QUANTIZE)quantize);
    
                FreeImage_SaveU(FIF_GIF, gif, oFile.c_str());
                FreeImage_Unload(pic);
                FreeImage_Unload(bmp);
                FreeImage_Unload(gif);
            }
            DeleteFile(iFile.c_str());

            //Create memory for gif
            ZeroMemory(&glsd, sizeof(GIF_LOGICAL_SCREEN_DESCRIPTOR));
		    ZeroMemory(&ggce, sizeof(GIF_GRAPHIC_CONTROL_EXTENSION));
		    ZeroMemory(&gid, sizeof(GIF_IMAGE_DESCRIPTOR));

		    hFileIn = CreateFile(oFile.c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
		    if (hFileIn==INVALID_HANDLE_VALUE)
            {
			    std::cout << "Could not open file. GIF creation aborted." << std::endl;
			    CloseHandle(hFileOut);
			    return -1;
		    }

            ReadFile(hFileIn, &gh, sizeof(GIF_HEADER), &dw, 0);
		    if (strncmp(gh.cSignature, "GIF", 3) || (strncmp(gh.cVersion, "89a", 3) && strncmp(gh.cVersion, "87a", 3)))
            {
			    std::cout << "Not a GIF file, or incorrect version number." << std::endl;
			    CloseHandle(hFileIn);
			    CloseHandle(hFileOut);
			    return -1;
		    }

            ReadFile(hFileIn,&glsd,sizeof(GIF_LOGICAL_SCREEN_DESCRIPTOR),&dw,0);
		    if (glsd.iGctFlag) ReadFile(hFileIn,szColorTable,iGctSize[glsd.iSizeOfGct],&dw,0);
		    if (glsd1.iWidth<glsd.iWidth) glsd1.iWidth=glsd.iWidth;
		    if (glsd1.iHeight<glsd.iHeight) glsd1.iHeight=glsd.iHeight;

            while(true)
            {
			    ReadFile(hFileIn, &c, 1, &dw, 0);
			    if(dw == 0)
                {
                    std::cout << "Premature end of file encountered; no GIF image data present." << std::endl;
				    CloseHandle(hFileIn);
				    CloseHandle(hFileOut);
				    return -1;
			    }
			    if(c == 0x2C) {
				    ReadFile(hFileIn, &gid, sizeof(GIF_IMAGE_DESCRIPTOR), &dw, 0);
				    if (gid.iLctFlag) {
					    ReadFile(hFileIn,szColorTable,iGctSize[gid.iSizeOfLct],&dw,0);
				    }
                    else {
					    gid.iLctFlag=1;
					    gid.iSizeOfLct=glsd.iSizeOfGct;
				    }
				    break;
			    } else if (c==0x21) {
				    ReadFile(hFileIn,&c,1,&dw,0);
				    if (c==0xF9) {
					    ReadFile(hFileIn,&ggce,sizeof(GIF_GRAPHIC_CONTROL_EXTENSION),&dw,0);
				    }
                    else
                    {
					    while(true)
                        {
						    ReadFile(hFileIn, &c, 1, &dw, 0);
						    if(!c) break;
						    SetFilePointer(hFileIn, c, 0, FILE_CURRENT);
					    }
				    }
			    }
		    }

            ggce.iBlockSize=4;
		    ggce.iDelayTime=delay;//CHANGE LATER!!!
		    ggce.iDisposalMethod=2;
		    c=(char)0x21;
		    WriteFile(hFileOut,&c,1,&dw,0);
		    c=(char)0xF9;
		    WriteFile(hFileOut,&c,1,&dw,0);
		    WriteFile(hFileOut,&ggce,sizeof(GIF_GRAPHIC_CONTROL_EXTENSION),&dw,0);
		    c=(char)0x2C;
		    WriteFile(hFileOut,&c,1,&dw,0);
		    WriteFile(hFileOut,&gid,sizeof(GIF_IMAGE_DESCRIPTOR),&dw,0);
		    WriteFile(hFileOut,szColorTable,iGctSize[gid.iSizeOfLct],&dw,0);
		    ReadFile(hFileIn,&c,1,&dw,0);
		    WriteFile(hFileOut,&c,1,&dw,0);
		    while(true)
            {
			    ReadFile(hFileIn,&c,1,&dw,0);
			    WriteFile(hFileOut,&c,1,&dw,0);
			    if (!c) break;
			    ReadFile(hFileIn,szColorTable,c,&dw,0);
			    WriteFile(hFileOut,szColorTable,c,&dw,0);
		    }
		    CloseHandle(hFileIn);
            DeleteFile(oFile.c_str());
        }
        c=(char)0x3B;
	    WriteFile(hFileOut,&c,1,&dw,0);
	    SetFilePointer(hFileOut,6,0,FILE_BEGIN);
	    WriteFile(hFileOut,&glsd1,sizeof(GIF_LOGICAL_SCREEN_DESCRIPTOR),&dw,0);
	    CloseHandle(hFileOut);

        std::cout << "Conversion has completed successfully!" << std::endl;
	    return 0;
    }
}