#ifndef WRAPPERS_DRIVER_HPP
#define WRAPPERS_DRIVER_HPP

#include "hardware.hpp"

#include <algorithm> // std::max

/**
 * @brief SH1106
 * Notes to how it works
 * 
 * Buffer to display mapping:
 * This seems to be arranged in a manner like
 *         ↓ bits
 * bytes → 1 2 3 .. 128
 *         2 2 2
 *         ..
 *         7 7 7
 *         8 8 8
 *         1 1 1
 *         ..
 * 
 * Each vertical section of 8 bits is a page, there are 8 pages.
 */
namespace SH1106 {
    enum Dimensions : int {
        XMax    = 132,
        YMax    = 64,
        XOffset = 2,
        YOffset = 0,

        Width  = XMax - 2 * XOffset,
        Height = YMax,

        Size = Width * Height
    };

    namespace Registry {
        enum Commands : int {
            /*
             "Specifies column address of display RAM. Divide the column
              address into 4 higher bits and 4 lower bits. Set each of them
              into successions. When the microprocessor repeats to access to
              the display RAM, the column address counter is incremented
              during each access until address 131 is accessed. The page
              address is not changed during this time." (spec p.19)

              Note LowColumn being 2 due to 128<->132 pixel discrepancy.
             */
            SelectColumnLow  = 0x02, // ( 0x0 -  0xF)
            SelectColumnHigh = 0x10, // (0x10 - 0x1F)

            /*
              Unknown, found in demo code.
             */
            // (0x00-0x02)
            SetPageAddressingMode = 0x20,

            /*
              "Specifies output voltage (VPP) of the internal charger pump."
              (spec p.19)

              0x30: 6.4V, 0x31: 7.4V, 0x32: 8.0V (Power On), 0x33: 9.0V
             */
            PumpVoltage = 0x30, // (0x30-0x33)

            /*
              "Specifies line address (refer to Figure. 8) to determine the
              initial display line or COM0. The RAM display data becomes the
              top line of OLED screen. It is followed by the higher number of
              lines in ascending order, corresponding to the duty cycle. When
              this command changes the line address, the smooth scrolling or
              page change takes place." (spec p.20)
             */
            SelectLine = 0x40, // (0x40 - 0x7F)

            /*
              "This command is to set contrast setting of the display. The
              chip has 256 contrast steps from 00 to FF. The segment output
              current increases as the contrast step value increases.
              Segment output current setting: ISEG = a/256 × IREF × scale
              factor, where: a is contrast step; IREF is reference current
              equals 12.5μA; Scale factor = 16." (spec p.20)
            
              This is a double-byte ("set") command. The next input will be
              used as value for the register. (range: 0x00-0xFF)
             */
            SetContrastControl        = 0x81,
            ValueContrastControlReset = 0x80,

            /*
              "Change the relationship between RAM column address and
              segment driver. The order of segment driver output pads can be
              reversed by software. This allows flexible IC layout during
              OLED module assembly. For details, refer to the column address
              section of Figure. 8. When display data is written or read, the
              column address is incremented by 1 as shown in Figure. 1."
              (spec p.21)
             */
            SegmentRemapNormal  = 0xA0,
            SegmentRemapReverse = 0xA1,

            /*
              "Forcibly turns the entire display on regardless of the
              contents of the display data RAM. At this time, the contents
              of the display data RAM are held." (spec p.21)
             */
            DisableForceDisplayOn = 0xA4,
            EnableForceDisplayOn  = 0xA5,

            /*
              "Reverses the display ON/OFF status without rewriting the
              contents of the display data RAM." (spec p.21)
             */
            DisableInverseDisplay = 0xA6,
            EnableInverseDisplay  = 0xA7,

            /*
              "This command switches default 64 multiplex modes to any
              multiplex ratio from 1 to 64. The output pads COM0-COM63 will
              be switched to corresponding common signal." (spec p.22)
            
              This is a double-byte ("set") command. The next input will be
              used as value for the register. (range: 0x00-0x3F)
             */
            SetMultiplexRatio          = 0xA8,
            ValueMultiplexRatioDefault = 0x3F,

            /*
              "This command is to control the DC-DC voltage converter. The
              converter will be turned on by issuing this command then
              display ON command. The panel display must be off while issuing
              this command." (spec p.22)
              
              This is a double-byte ("set") command. The next input will be
              used as value for the register. (range: 0x8A-0x8B)
              
              (Why did they make a setter for a 0-1 range?)
             */
            SetDCDC = 0xAD,

            /*
              "Alternatively turns the display on and off." (spec p. 23)
             */
            PanelOff = 0xAE,
            PanelOn  = 0xAF,

            /*
              "Specifies page address to load display RAM data to page
              address register. Any RAM data bit can be accessed when its
              page address and column address are specified. The display
              remains unchanged even when the page address is changed."
              (spec p.23)
             */
            Page = 0xB0, // 0xB0 - 0xB7

            /*
              "This command sets the scan direction of the common output
              allowing layout flexibility in OLED module design. In addition,
              the display will have immediate effect once this command is
              issued. That is, if this command is sent during normal display,
              the graphic display will be vertically flipped." (spec p.24)
            
              "When D = “L”, Scan from COM0 to COM [N -1]. (POR)
              When D = “H”, Scan from COM [N -1] to COM0." (spec p.24)
              Where D = 4th significant bit
             */
            ComRowScanDirection = 0xC0, // (0xC0-0xC8), (0xC9-0xCF for reversed?)

            // unused: 0xC1-0xCF

            /*
              "The next command specifies the mapping of display start line
              to one of COM0-63 (it is assumed that COM0 is the display start
              line, that equals to 0). For example, to move the COM16 towards
              the COM0 direction for 16 lines, the 6-bit data in the second
              byte should be given by 010000. To move in the opposite
              direction by 16 lines, the 6-bit data should be given by
              (64-16), so the second byte should be 100000." (spec p.24)
            
              This is a double-byte ("set") command. The next input will be
              used as value for the register. (range: 0x00-0x3F)
             */
            SetDisplayOffset          = 0xD3,
            ValueDisplayOffsetDefault = 0x0,

            /*
              "This command is used to set the frequency of the internal
              display clocks (DCLKs). It is defined as the divide ratio
              (Value from 1 to 16) used to divide the oscillator frequency.
              POR is 1. Frame frequency is determined by divide ratio, number
              of display clocks per row, MUX ratio and oscillator frequency."
              (spec p.25)
            
              This is a double-byte ("set") command. The next input will be
              used as value for the register.
              (range: 0x0-0xF divide ratio + 0x10-0xF0 oscillator frequency)
             */
            SetDisplayClockFreq          = 0xD5,
            ValueDisplayClockFreqDefault = 0b01010000,

            /*
              "This command is used to set the duration of the pre-charge
              period. The interval is counted in number of DCLK. POR is 2
              DCLKs." (spec p.26)
            
              This is a double-byte ("set") command. The next input will be
              used as value for the register.
              (range: 0x0-0xF precharge period + 
                      0x10-0xF0 discharge period) 
             */
            SetChargePeriod          = 0xD9,
            ValueChargePeriodDefault = 0x22,

            /*
              "This command is to set the common signals pad configuration
              (sequential or alternative) to match the OLED panel hardware
              layout" (spec p.26)
            
              This is a double-byte ("set") command. The next input will be
              used as value for the register. (range: 0x02, 0x12)
             */
            SetComPinsHWConf          = 0xDA,
            ValueComPinsHWConfDefault = 0x12,

            /*
              "This command is to set the common pad output voltage level
              at deselect stage." (spec p.27)
            
              This is a double-byte ("set") command. The next input will be
              used as value for the register. (range: 0x00-0xFF)
            
              Resulting VCOM = β × VREF = (0.430 + <value> × 0.006415) × VREF
             */
            SetVCOMH          = 0xDB,
            ValueVCOMHDefault = 0x35,

            /*
              "A pair of Read-Modify-Write and End commands must always be
              used. Once read-modify-write is issued, column address is not
              incremental by read display data command but incremental by
              write display data command only. It continues until End command
              is issued. When the End is issued, column address returns to
              the address when read-modify-write is issued. This can reduce
              the microprocessor load when data of a specific display area
              is repeatedly changed during cursor blinking or others."
              (spec p.28)
             */
            ReadModifyWriteOn  = 0xE0,
            ReadModifyWriteOff = 0xEE,

            /*
              "Non-Operation Command." (spec p.29)
             */
            Nop = 0xE3
        };
    }
} // namespace SH1106

namespace SSD1305 {
    enum Dimensions : int {
        XMax    = 128,
        YMax    = 32,
        XOffset = 0,
        YOffset = 0,

        Width  = XMax,
        Height = YMax,

        Size = Width * Height
    };

    namespace Registry {
        /**
         * Commands as per SSD1305 spec Rev 1.9
         */
        enum Commands : int {
            // p. 40
            SelectColumnLow  = 0x00, // [0x0-0xF]
            SelectColumnHigh = 0x10, // [0x0-0xF]

            SetMemoryAddressingMode = 0x20,

            // p. 43
            SetDisplayStartLine = 0x40,
            SetContrastControl  = 0x81,

            // p. 44
            SetSegmentRemap = 0xA0, // [0x0-0x1]

            DisableInverseDisplay = 0xA6,
            EnableInverseDisplay  = 0xA7,

            SetMultiplexRatio = 0xA8,

            // p. 45
            PanelDim = 0xAC,
            PanelOff = 0xAE,
            PanelOn  = 0xAF,

            SetComOutputScanDir = 0xC0 + 0x8,

            SetDisplayOffset = 0xD3,

            // also known as clock divide ratio
            SetDisplayOscillatorFrequency = 0xD5,

            SetAreaColorMode = 0xD8,

            SetPrechargePeriod              = 0xD9,
            SetCOMPinsHardwareConfiguration = 0xDA,
            SetVComH                        = 0xDB
        };
    } // namespace Registry
} // namespace SSD1305

/**
 * @brief SSD1322
 * 
 * Pixels are 4 bits, a byte contains 2 adjacent pixels-
 * 
 */
namespace SSD1322 {
    enum Dimensions : int {
        XMax    = 256,
        YMax    = 64,
        XOffset = 0,
        YOffset = 0,

        Width  = XMax,
        Height = YMax,

        Size = Width * Height
    };

    namespace Registry {
        enum Commands : int {
            SetColumnAddress = 0x15,
            SetRowAddress    = 0x75,

            WriteRam = 0x5C,

            SetRemap                 = 0xA0,
            SetStartLine             = 0xA1,
            SetDisplayOffset         = 0xA2,
            DisableForceDisplayOn    = 0xA4,
            EnableForceDisplayOn     = 0xA5,
            DisableInverseDisplay    = 0xA6,
            EnableInverseDisplay     = 0xA7,
            ExitPartialDispaly       = 0xA9,
            FunctionSelect           = 0xAB,
            PanelOff                 = 0xAE,
            PanelOn                  = 0xAF,
            SetPhaseLength           = 0xB1,
            SetClockDivider          = 0xB3,
            DisplayEnhance           = 0xB4,
            SetGPIO                  = 0xB5,
            SetSecondPrechargePeriod = 0xB6,
            SelectDefaultGrayscale   = 0xB9,
            SetPrechargeVoltage      = 0xBB,
            SetVComH                 = 0xBE,
            SetContrastCurrent       = 0xC1,
            MasterCurrentControl     = 0xC7,
            SetMultiplexRatio        = 0xCA,
            DisplayEnhanceB          = 0xD1,
            SetCommandLock           = 0xFD,

        };
    }
} // namespace SSD1322

namespace Wrappers {
    /**
     * @brief Abstraction for basic program-display-controller interaction
     */
    class Driver {
        public:
        using ColorT = uint16_t;

        enum class ScanDirection {
            LeftRight_UpDown = 0,
            LeftRight_DownUp,
            RightLeft_UpDown,
            RightLeft_DownUp,

            UpDown_LeftRight,
            UpDown_RightLeft,
            DownUp_LeftRight,
            DownUp_RightLeft,

            Default = LeftRight_UpDown
        };

        struct Pins {
            enum PinIndices : int {
                KeyUpPin    = 6,
                KeyDownPin  = 19,
                KeyLeftPin  = 5,
                KeyRightPin = 26,
                KeyPressPin = 13,
                Key1Pin     = 21,
                Key2Pin     = 20,
                Key3Pin     = 16
            };
            enum I2CCommands : int {
                IICCMD = 0x00,
                IICRAM = 0x40
            };
        };

        enum class Mode {
            SH1106,
            SSD1305,
            SSD1322
        };

        explicit Driver(Mode mode, ScanDirection scanDir = ScanDirection::Default);
        ~Driver();
        void SetScanDirection(ScanDirection scanDir);

        void SetCursor(uint8_t x, uint8_t y);
        void SetColor(uint8_t x, uint8_t y, ColorT color);

        void Clear(ColorT color = 0);
        void Display();
        void SetPanelPower(bool on = true);

        void CopyGLBuffer(const uint8_t *glBuffer);

        uint8_t GetKeyUp();
        uint8_t GetKeyDown();
        uint8_t GetKeyLeft();
        uint8_t GetKeyRight();
        uint8_t GetKeyPress();
        uint8_t GetKey1();
        uint8_t GetKey2();
        uint8_t GetKey3();

        [[nodiscard]] int GetWidth() const;
        [[nodiscard]] int GetHeight() const;

        private:
        void Reset();
        void InitRegistry();
        void WriteRegistry(uint8_t reg);
        void WriteDataByte(uint8_t data);
        void WriteData(uint8_t *buffer, uint32_t length);

        // 1-4 bpp buffer
        static constexpr auto _bufferSize{ std::max({ static_cast<size_t>(SH1106::Size / 8), static_cast<size_t>(SSD1305::Size / 8), static_cast<size_t>(SSD1322::Size / 2) }) };
        uint8_t _buffer[_bufferSize];
        struct {
            int width{};
            int height{};
            ScanDirection scanDir{ ScanDirection::Default };
        } _state;

        Mode _mode{ Mode::SSD1322 };
    };
} // namespace Wrappers

#endif