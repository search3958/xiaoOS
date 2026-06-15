#ifndef XIAO_EFI_H
#define XIAO_EFI_H

#ifdef _MSC_VER
#define EFIAPI __cdecl
#else
#define EFIAPI __attribute__((ms_abi))
#endif

typedef unsigned short CHAR16;
typedef unsigned long long UINTN;
typedef unsigned long long EFI_STATUS;
typedef void *EFI_HANDLE;
typedef unsigned int UINT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;
typedef unsigned char BOOLEAN;
typedef long long INT64;

#define EFI_SUCCESS 0

typedef struct {
    unsigned long long Signature;
    unsigned int Revision;
    unsigned int HeaderSize;
    unsigned int CRC32;
    unsigned int Reserved;
} EFI_TABLE_HEADER;

typedef struct {
    UINT32 Data1;
    UINT16 Data2;
    UINT16 Data3;
    UINT8 Data4[8];
} EFI_GUID;

/* EFI_USB_IO_PROTOCOL GUID {2B2F68D8-8096-4861-92F6-96D56E7B4F35} */
static const EFI_GUID usb_io_guid = {
    0x2b2f68d8, 0x8096, 0x4861, {0x92, 0xf6, 0x96, 0xd5, 0x6e, 0x7b, 0x4f, 0x35}
};

struct EFI_USB_IO_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_USB_CONTROL_TRANSFER)(
    struct EFI_USB_IO_PROTOCOL *This,
    void *Request,
    UINTN Direction,
    UINTN Timeout,
    void *Data,
    UINTN DataLength,
    UINT32 *Status
);

typedef EFI_STATUS (EFIAPI *EFI_USB_SYNC_INTERRUPT_TRANSFER)(
    struct EFI_USB_IO_PROTOCOL *This,
    UINT8 DeviceEndpoint,
    UINT8 *Data,
    UINTN *DataLength,
    UINTN Timeout,
    UINT32 *Status
);

typedef struct EFI_USB_IO_PROTOCOL {
    EFI_USB_CONTROL_TRANSFER UsbControlTransfer;
    void *UsbBulkTransfer;
    EFI_USB_SYNC_INTERRUPT_TRANSFER UsbSyncInterruptTransfer;
    void *UsbAsyncInterruptTransfer;
    void *UsbIsochronousTransfer;
    void *UsbAsyncIsochronousTransfer;
    void *UsbGetDeviceDescriptor;
    void *UsbGetConfigDescriptor;
    void *UsbGetInterfaceDescriptor;
    void *UsbGetEndpointDescriptor;
    void *UsbGetStringDescriptor;
    void *UsbGetSupportedLanguageDescriptor;
    void *UsbDeviceRequestTransfer;
    void *GetDeviceSpeed;
    void *PortReset;
} EFI_USB_IO_PROTOCOL;

struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;
struct SIMPLE_TEXT_OUTPUT_MODE;

typedef EFI_STATUS (EFIAPI *EFI_TEXT_STRING)(
    struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self,
    CHAR16 *string
);

typedef EFI_STATUS (EFIAPI *EFI_TEXT_CLEAR_SCREEN)(
    struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self
);

typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_CURSOR_POSITION)(
    struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self,
    UINTN column,
    UINTN row
);

typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_ATTRIBUTE)(
    struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self,
    UINTN attribute
);

typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    void *Reset;
    EFI_TEXT_STRING OutputString;
    void *TestString;
    void *QueryMode;
    void *SetMode;
    EFI_TEXT_SET_ATTRIBUTE SetAttribute;
    EFI_TEXT_CLEAR_SCREEN ClearScreen;
    EFI_TEXT_SET_CURSOR_POSITION SetCursorPosition;
    void *EnableCursor;
    struct SIMPLE_TEXT_OUTPUT_MODE *Mode;
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_LOCATE_PROTOCOL)(
    EFI_GUID *Protocol,
    void *Registration,
    void **Interface
);

typedef EFI_STATUS (EFIAPI *EFI_FREE_POOL)(
    void *Buffer
);

typedef EFI_STATUS (EFIAPI *EFI_STALL)(
    UINTN Microseconds
);

typedef EFI_STATUS (EFIAPI *EFI_LOCATE_HANDLE_BUFFER)(
    UINTN SearchType,
    EFI_GUID *Protocol,
    void *SearchKey,
    UINTN *NoHandles,
    EFI_HANDLE **Buffer
);

typedef EFI_STATUS (EFIAPI *EFI_OPEN_PROTOCOL)(
    EFI_HANDLE Handle,
    EFI_GUID *Protocol,
    void **Interface,
    EFI_HANDLE AgentHandle,
    EFI_HANDLE ControllerHandle,
    UINT32 Attributes
);

typedef EFI_STATUS (EFIAPI *EFI_HANDLE_PROTOCOL)(
    EFI_HANDLE Handle,
    EFI_GUID *Protocol,
    void **Interface
);

typedef EFI_STATUS (EFIAPI *EFI_CONNECT_CONTROLLER)(
    EFI_HANDLE ControllerHandle,
    EFI_HANDLE *DriverImageHandle,
    void *RemainingDevicePath,
    BOOLEAN Recursive
);

typedef struct EFI_BOOT_SERVICES {
    EFI_TABLE_HEADER Hdr;
    void *RaiseTPL;
    void *RestoreTPL;
    void *AllocatePages;
    void *FreePages;
    void *GetMemoryMap;
    void *AllocatePool;
    EFI_FREE_POOL FreePool;
    void *CreateEvent;
    void *SetTimer;
    void *WaitForEvent;
    void *SignalEvent;
    void *CloseEvent;
    void *CheckEvent;
    void *InstallProtocolInterface;
    void *ReinstallProtocolInterface;
    void *UninstallProtocolInterface;
    EFI_HANDLE_PROTOCOL HandleProtocol;
    void *Reserved;
    void *RegisterProtocolNotify;
    void *LocateHandle;
    void *LocateDevicePath;
    void *InstallConfigurationTable;
    void *LoadImage;
    void *StartImage;
    void *Exit;
    void *UnloadImage;
    void *ExitBootServices;
    void *GetNextMonotonicCount;
    EFI_STALL Stall;
    void *SetWatchdogTimer;
    EFI_CONNECT_CONTROLLER ConnectController;
    void *DisconnectController;
    EFI_OPEN_PROTOCOL OpenProtocol;
    void *CloseProtocol;
    void *OpenProtocolInformation;
    void *ProtocolsPerHandle;
    EFI_LOCATE_HANDLE_BUFFER LocateHandleBuffer;
    EFI_LOCATE_PROTOCOL LocateProtocol;
    void *InstallMultipleProtocolInterfaces;
    void *UninstallMultipleProtocolInterfaces;
    void *CalculateCrc32;
    void *CopyMem;
    void *SetMem;
    void *CreateEventEx;
} EFI_BOOT_SERVICES;

typedef enum {
    PixelRedGreenBlueReserved8BitPerColor,
    PixelBlueGreenRedReserved8BitPerColor,
    PixelBitMask,
    PixelBltOnly,
    PixelFormatMax
} EFI_GRAPHICS_PIXEL_FORMAT;

typedef struct {
    UINT32 RedMask;
    UINT32 GreenMask;
    UINT32 BlueMask;
    UINT32 ReservedMask;
} EFI_PIXEL_BITMASK;

typedef struct {
    UINT32 Version;
    UINT32 HorizontalResolution;
    UINT32 VerticalResolution;
    EFI_GRAPHICS_PIXEL_FORMAT PixelFormat;
    EFI_PIXEL_BITMASK PixelInformation;
    UINT32 PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE {
    UINT32 MaxMode;
    UINT32 Mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    UINTN SizeOfInfo;
    UINTN FrameBufferBase;
    UINTN FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

typedef struct {
    UINT8 Blue;
    UINT8 Green;
    UINT8 Red;
    UINT8 Reserved;
} EFI_GRAPHICS_OUTPUT_BLT_PIXEL;

typedef enum {
    EfiBltVideoFill,
    EfiBltVideoToBltBuffer,
    EfiBltBufferToVideo,
    EfiBltVideoToVideo,
    EfiGraphicsOutputBltOperationMax
} EFI_GRAPHICS_OUTPUT_BLT_OPERATION;

struct EFI_GRAPHICS_OUTPUT_PROTOCOL;
typedef EFI_STATUS (*EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE)(
    struct EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    UINT32 ModeNumber,
    UINTN *SizeOfInfo,
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info
);
typedef EFI_STATUS (*EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE)(
    struct EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    UINT32 ModeNumber
);
typedef EFI_STATUS (*EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT)(
    struct EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer,
    EFI_GRAPHICS_OUTPUT_BLT_OPERATION BltOperation,
    UINTN SourceX,
    UINTN SourceY,
    UINTN DestinationX,
    UINTN DestinationY,
    UINTN Width,
    UINTN Height,
    UINTN Delta
);

typedef struct EFI_GRAPHICS_OUTPUT_PROTOCOL {
    EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE QueryMode;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE SetMode;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT Blt;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode;
} EFI_GRAPHICS_OUTPUT_PROTOCOL;

typedef struct {
    INT64 RelativeMovementX;
    INT64 RelativeMovementY;
    INT64 RelativeMovementZ;
    BOOLEAN LeftButton;
    BOOLEAN RightButton;
} EFI_SIMPLE_POINTER_STATE;

typedef struct {
    unsigned long long ResolutionX;
    unsigned long long ResolutionY;
    unsigned long long ResolutionZ;
    BOOLEAN LeftButton;
    BOOLEAN RightButton;
} EFI_SIMPLE_POINTER_MODE;

struct EFI_SIMPLE_POINTER_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_SIMPLE_POINTER_RESET)(
    struct EFI_SIMPLE_POINTER_PROTOCOL *This,
    BOOLEAN ExtendedVerification
);

typedef EFI_STATUS (EFIAPI *EFI_SIMPLE_POINTER_GET_STATE)(
    struct EFI_SIMPLE_POINTER_PROTOCOL *This,
    EFI_SIMPLE_POINTER_STATE *State
);

typedef struct EFI_SIMPLE_POINTER_PROTOCOL {
    EFI_SIMPLE_POINTER_RESET Reset;
    EFI_SIMPLE_POINTER_GET_STATE GetState;
    void *WaitForInput;
    EFI_SIMPLE_POINTER_MODE *Mode;
} EFI_SIMPLE_POINTER_PROTOCOL;

typedef struct {
    EFI_TABLE_HEADER Hdr;
    CHAR16 *FirmwareVendor;
    unsigned int FirmwareRevision;
    EFI_HANDLE ConsoleInHandle;
    void *ConIn;
    EFI_HANDLE ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
    EFI_HANDLE StandardErrorHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *StdErr;
    void *RuntimeServices;
    EFI_BOOT_SERVICES *BootServices;
    UINTN NumberOfTableEntries;
    void *ConfigurationTable;
} EFI_SYSTEM_TABLE;

/* ------------------------------------------------------------------ */
/* EFI_ABSOLUTE_POINTER_PROTOCOL  (usb-tablet / virtio-tablet)        */
/* ------------------------------------------------------------------ */
typedef struct {
    UINTN AbsoluteMinX;
    UINTN AbsoluteMinY;
    UINTN AbsoluteMinZ;
    UINTN AbsoluteMaxX;
    UINTN AbsoluteMaxY;
    UINTN AbsoluteMaxZ;
    UINT32 Attributes;
} EFI_ABSOLUTE_POINTER_MODE;

typedef struct {
    UINTN CurrentX;
    UINTN CurrentY;
    UINTN CurrentZ;
    UINT32 ActiveButtons;
} EFI_ABSOLUTE_POINTER_STATE;

struct EFI_ABSOLUTE_POINTER_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_ABSOLUTE_POINTER_RESET)(
    struct EFI_ABSOLUTE_POINTER_PROTOCOL *This,
    BOOLEAN ExtendedVerification
);

typedef EFI_STATUS (EFIAPI *EFI_ABSOLUTE_POINTER_GET_STATE)(
    struct EFI_ABSOLUTE_POINTER_PROTOCOL *This,
    EFI_ABSOLUTE_POINTER_STATE *State
);

typedef struct EFI_ABSOLUTE_POINTER_PROTOCOL {
    EFI_ABSOLUTE_POINTER_RESET Reset;
    EFI_ABSOLUTE_POINTER_GET_STATE GetState;
    void *WaitForInput;
    EFI_ABSOLUTE_POINTER_MODE *Mode;
} EFI_ABSOLUTE_POINTER_PROTOCOL;

#endif
