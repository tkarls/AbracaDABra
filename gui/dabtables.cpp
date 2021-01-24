#include "dabtables.h"

const QMap<uint32_t, QString> DabTables::channelList =
{
    {174928 , "5A"},
    {176640 , "5B"},
    {178352 , "5C"},
    {180064 , "5D"},
    {181936 , "6A"},
    {183648 , "6B"},
    {185360 , "6C"},
    {187072 , "6D"},
    {188928 , "7A"},
    {190640 , "7B"},
    {192352 , "7C"},
    {194064 , "7D"},
    {195936 , "8A"},
    {197648 , "8B"},
    {199360 , "8C"},
    {201072 , "8D"},
    {202928 , "9A"},
    {204640 , "9B"},
    {206352 , "9C"},
    {208064 , "9D"},
    {209936 , "10A"},
    {211648 , "10B"},
    {213360 , "10C"},
    {215072 , "10D"},
    #if RADIO_CONTROL_N_CHANNELS_ENABLE
    {210096 , "10N"},
    #endif
    {216928 , "11A"},
    {218640 , "11B"},
    {220352 , "11C"},
    {222064 , "11D"},
    #if RADIO_CONTROL_N_CHANNELS_ENABLE
    {217088 , "11N"},
    #endif
    {223936 , "12A"},
    {225648 , "12B"},
    {227360 , "12C"},
    {229072 , "12D"},
    #if RADIO_CONTROL_N_CHANNELS_ENABLE
    {224096 , "12N"},
    #endif
    {230784 , "13A"},
    {232496 , "13B"},
    {234208 , "13C"},
    {235776 , "13D"},
    {237488 , "13E"},
    {239200 , "13F"}
};

const QStringList DabTables::PTyNames =
{
    "None",
    "News",
    "Current Affairs",
    "Information",
    "Sport",
    "Education",
    "Drama",
    "Culture",
    "Science",
    "Varied",
    "Pop Music",
    "Rock Music",
    "Easy Listening Music",
    "Light Classical",
    "Serious Classical",
    "Other Music",
    "Weather/meteorology",
    "Finance/Business",
    "Children's programmes",
    "Social Affairs",
    "Religion",
    "Phone In",
    "Travel",
    "Leisure",
    "Jazz Music",
    "Country Music",
    "National Music",
    "Oldies Music",
    "Folk Music",
    "Documentary",
    "",
    "",
};

const uint16_t DabTables::ebuLatin2UCS2[] =
{   /* UCS2 == UTF16 for Basic Multilingual Plane 0x0000-0xFFFF */
    0x0000, 0x0118, 0x012E, 0x0172, 0x0102, 0x0116, 0x010E, 0x0218, /* 0x00 - 0x07 */
    0x021A, 0x010A, 0x000A, 0x000B, 0x0120, 0x0139, 0x017B, 0x0143, /* 0x08 - 0x0f */
    0x0105, 0x0119, 0x012F, 0x0173, 0x0103, 0x0117, 0x010F, 0x0219, /* 0x10 - 0x17 */
    0x021B, 0x010B, 0x0147, 0x011A, 0x0121, 0x013A, 0x017C, 0x001F, /* 0x18 - 0x1f */
    0x0020, 0x0021, 0x0022, 0x0023, 0x0142, 0x0025, 0x0026, 0x0027, /* 0x20 - 0x27 */
    0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F, /* 0x28 - 0x2f */
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, /* 0x30 - 0x37 */
    0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F, /* 0x38 - 0x3f */
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, /* 0x40 - 0x47 */
    0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, /* 0x48 - 0x4f */
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, /* 0x50 - 0x57 */
    0x0058, 0x0059, 0x005A, 0x005B, 0x016E, 0x005D, 0x0141, 0x005F, /* 0x58 - 0x5f */
    0x0104, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, /* 0x60 - 0x67 */
    0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F, /* 0x68 - 0x6f */
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, /* 0x70 - 0x77 */
    0x0078, 0x0079, 0x007A, 0x00AB, 0x016F, 0x00BB, 0x013D, 0x0126, /* 0x78 - 0x7f */
    0x00E1, 0x00E0, 0x00E9, 0x00E8, 0x00ED, 0x00EC, 0x00F3, 0x00F2, /* 0x80 - 0x87 */
    0x00F , 0x00F9, 0x00D1, 0x00C7, 0x015E, 0x00DF, 0x00A1, 0x0178, /* 0x88 - 0x8f */
    0x00E2, 0x00E4, 0x00EA, 0x00EB, 0x00EE, 0x00EF, 0x00F4, 0x00F6, /* 0x90 - 0x97 */
    0x00FB, 0x00FC, 0x00F1, 0x00E7, 0x015F, 0x011F, 0x0131, 0x00FF, /* 0x98 - 0x9f */
    0x0136, 0x0145, 0x00A9, 0x0122, 0x011E, 0x011B, 0x0148, 0x0151, /* 0xa0 - 0xa7 */
    0x0150, 0x20AC, 0x00A3, 0x0024, 0x0100, 0x0112, 0x012A, 0x016A, /* 0xa8 - 0xaf */
    0x0137, 0x0146, 0x013B, 0x0123, 0x013C, 0x0130, 0x0144, 0x0171, /* 0xb0 - 0xb7 */
    0x0170, 0x00BF, 0x013E, 0x00B0, 0x0101, 0x0113, 0x012B, 0x016B, /* 0xb8 - 0xbf */
    0x00C1, 0x00C0, 0x00C9, 0x00C8, 0x00CD, 0x00CC, 0x00D3, 0x00D2, /* 0xc0 - 0xc7 */
    0x00DA, 0x00D9, 0x0158, 0x010C, 0x0160, 0x017D, 0x00D0, 0x013F, /* 0xc8 - 0xcf */
    0x00C2, 0x00C4, 0x00CA, 0x00CB, 0x00CE, 0x00CF, 0x00D4, 0x00D6, /* 0xd0 - 0xd7 */
    0x00DB, 0x00DC, 0x0159, 0x010D, 0x0161, 0x017E, 0x0111, 0x0140, /* 0xd8 - 0xdf */
    0x00C3, 0x00C5, 0x00C6, 0x0152, 0x0177, 0x00DD, 0x00D5, 0x00D8, /* 0xe0 - 0xe7 */
    0x00DE, 0x014A, 0x0154, 0x0106, 0x015A, 0x0179, 0x0164, 0x00F0, /* 0xe8 - 0xef */
    0x00E3, 0x00E5, 0x00E6, 0x0153, 0x0175, 0x00FD, 0x00F5, 0x00F8, /* 0xf0 - 0xf7 */
    0x00FE, 0x014B, 0x0155, 0x0107, 0x015B, 0x017A, 0x0165, 0x0127, /* 0xf8 - 0xff */
};


//DabTables::DabTables()
//{

//}

QString DabTables::convertToQString(const char *c, uint8_t charset, uint8_t len)
{
    QString out;
    switch (static_cast<DabCharset>(charset))
    {
    case DabCharset::UTF8:
        out = out.fromUtf8(c);
        break;
    case DabCharset::UCS2:
    {
        uint8_t ucsLen = len/2+1;
        uint16_t * temp = new uint16_t[ucsLen];
        // BOM = 0xFEFF, DAB label is in big endian, writing it byte by byte
        uint8_t * bomPtr = (uint8_t *) temp;
        *bomPtr++ = 0xFE;
        *bomPtr++ = 0xFF;
        memcpy(temp+1, c, len);
        out = out.fromUtf16(temp, ucsLen);
        delete [] temp;
    }
        break;
    case DabCharset::EBULATIN:
        while(*c)
        {
            out.append(QChar(ebuLatin2UCS2[uint8_t(*c++)]));
        }
        break;
    default:
        // do noting, unsupported charset
        qDebug("ERROR: Charset %d is not supported", charset);
        break;
    }

    return out;
}


QString DabTables::getPtyName(const uint8_t pty)
{
    if (pty >= PTyNames.size())
    {
        return PTyNames[0];
    }
    return PTyNames[pty];
}
