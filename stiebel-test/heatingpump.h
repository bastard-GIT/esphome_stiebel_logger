#if !defined(heatingpump_H)
#define heatingpump_H
#include "ElsterTable.h"
#include "KElsterTable.h"
#include <sstream>
#include <iomanip>

typedef struct
{
    const char *Name;
    uint32_t CanId;
    uint8_t ReadId[2];
    uint8_t WriteId[2];
    uint8_t ConfirmationId[2];
} CanMember;

static const CanMember CanMembers[] =
    {
        // TODO: Read/Write-IDs gerade ziehen
        //  Name  CanId     ReadId          WriteId         ConfirmationID
        {"KESSEL",        0x180, {0x31, 0x00}, {0x30, 0x00}, {0x00, 0x00}},
        {"SPEICHER",      0x201, {0x41, 0x01}, {0x40, 0x01}, {0x00, 0x00}},
        {"ATEZ",          0x280, {0x51, 0x00}, {0x50, 0x00}, {0x00, 0x00}},
        {"BEDIENMODUL_1", 0x300, {0x61, 0x00}, {0x60, 0x00}, {0x00, 0x00}},
        {"BEDIENMODUL_2", 0x301, {0x61, 0x01}, {0x60, 0x01}, {0x00, 0x00}},
        {"BEDIENMODUL_3", 0x302, {0x61, 0x02}, {0x60, 0x02}, {0x00, 0x00}},
        {"BEDIENMODUL_4", 0x303, {0x61, 0x03}, {0x60, 0x03}, {0x00, 0x00}},
        {"RAUMFERNFUEHLER", 0x400, {0x00, 0x00}, {0x00, 0x00}, {0x00, 0x00}},
        {"MANAGER",       0x480, {0x91, 0x00}, {0x90, 0x00}, {0x00, 0x00}},
        {"WPUMPE",        0x514, {0xA1, 0x14}, {0xA0, 0x14}, {0x00, 0x00}},
        {"HEIZMODUL", 0x500, {0xA1, 0x00}, {0xA0, 0x00}, {0x00, 0x00}},
        {"BUSKOPPLER", 0x580, {0xB1, 0x00}, {0xB0, 0x00}, {0x00, 0x00}},
        {"MISCHERMODUL_1", 0x600, {0xC1, 0x00}, {0xC0, 0x00}, {0x00, 0x00}},
        {"MISCHERMODUL_2", 0x601, {0xC1, 0x01}, {0xC0, 0x01}, {0x00, 0x00}},
        {"MISCHERMODUL_3", 0x602, {0xC1, 0x02}, {0xC0, 0x02}, {0x00, 0x00}},
        {"MISCHERMODUL_4", 0x603, {0xC1, 0x03}, {0xC0, 0x03}, {0x00, 0x00}},
        {"ESPCLIENT", 0x680, {0xD1, 0x00}, {0xD0, 0x00}, {0x00, 0x00}},
        {"FREMDGERAET", 0x700, {0xE1, 0x00}, {0xE0, 0x00}, {0x00, 0x00}},
        {"DCF_MODUL", 0x780, {0xF1, 0x00}, {0xF0, 0x00}, {0x00, 0x00}},
        {"OTHER", 0x000, { 0x01, 0x00}, { 0x00, 0x00}, {0x00, 0x00}}};

typedef enum
{
    // Die Reihenfolge muss mit CanMembers übereinstimmen!
    cm_kessel = 0,
    cm_speicher,
    cm_atez,
    cm_bedienmodul_1,
    cm_bedienmodul_2,
    cm_bedienmodul_3,
    cm_bedienmodul_4,
    cm_raumfernfuehler,
    cm_manager,
    cm_wpumpe,
    cm_heizmodul,
    cm_buskoppler,
    cm_mischermodul_1,
    cm_mischermodul_2,
    cm_mischermodul_3,
    cm_mischermodul_4,
    cm_espclient,
    cm_fremdgeraet,
    cm_dcf_modul,
    cm_other
} CanMemberType;

const CanMember &lookupCanMember(uint32_t canId)
{
    for (const CanMember &member : CanMembers)
    {
        if (member.CanId == canId)
        {
            return member;
        }
    }
    // Return the last element if member is not found
    return CanMembers[sizeof(CanMembers) / sizeof(CanMembers[0]) - 1];
}

CanMemberType lookupCanMemberType(uint32_t canId)
{
    for (size_t i = 0; i < sizeof(CanMembers) / sizeof(CanMember); ++i)
    {
        if (CanMembers[i].CanId == canId)
        {
            return static_cast<CanMemberType>(i);
        }
    }
    return cm_other; // Return cm_other if member is not found
}

const ElsterIndex *processCanMessage(const std::vector<uint8_t> &msg, uint32_t can_id, std::string &signalValue)
{
    unsigned int receiver_id;
    // Return if the message is too small
    if (msg.size() < 7)
    {
        return &ElsterTable[0];
    }

    const CanMember &cm = lookupCanMember(can_id);
    //receiver_id = 0x180;
    receiver_id = int((msg[1] & 0x0F)+( (msg[0] & 0xF0 )*8));
    const CanMember &receiver_cm = lookupCanMember(receiver_id);

    const ElsterIndex *ei;
    unsigned char byte1;
    unsigned char byte2;
    unsigned char message_type_id;
    unsigned int ElsterIndex_id;
    char charValue[16];
    char message_type[16];

    message_type_id = msg[0] & 0x0F;
    switch (message_type_id) {
    case 0:
        strcpy(message_type, "write");
        break;
    case 1:
        strcpy(message_type, "read");
        break;
    case 2:
        strcpy(message_type, "response");
        break;
    case 3:
        strcpy(message_type, "ack");
        break;
    case 4:
        strcpy(message_type, "write ack");
        break;
    case 5:
        strcpy(message_type, "write respond");
        break;
    case 6:
        strcpy(message_type, "system");
        break;
    case 7:
        strcpy(message_type, "system respond");
        break;
    default:
        strcpy(message_type, "unknown");
    }
    if (msg[2] == 0xfa)
    {
        byte1 = msg[5];
        byte2 = msg[6];
        ElsterIndex_id = int((msg[4])+( (msg[3])<<8));
        ei = GetElsterIndex(ElsterIndex_id);
    } else {
        byte1 = msg[3];
        byte2 = msg[4];
        ElsterIndex_id = int(msg[2]);
        ei = GetElsterIndex(ElsterIndex_id);
    }

    switch (ei->Type)
    {
    case et_double_val:
        SetDoubleType(charValue, ei->Type, static_cast<double>(byte2 + (byte1 << 8)));
        break;
    case et_triple_val:
        SetDoubleType(charValue, ei->Type, static_cast<double>(byte2 + (byte1 << 8)));
        break;
    default:
        SetValueType(charValue, ei->Type, static_cast<int>(byte2 + (byte1 << 8)));
        break;
    }

    ESP_LOGI("can_log", "0x%03X %02X %02X %02X %02X %02X %02X %02X : 0x%03X -> 0x%03X (%s):\t%s(0x%04X):\t%s\t(%s)",cm.CanId, msg[0], msg[1], msg[2], msg[3], msg[4], msg[5], msg[6], cm.CanId, receiver_cm.CanId, message_type, ei->Name, ElsterIndex_id, charValue, ElsterTypeStr[ei->Type]);
    // ESP_LOGI("can-test: ", "0x%02x -> 0x%02x ():\t%s(0x%04X):\t%s\t(%s)", cm.Name, cm.CanId, ei->Name, ElsterIndex_id, charValue, ElsterTypeStr[ei->Type]);
    //ESP_LOGI("processCanMessage()", "1: %s (0x%02x):\t%s(0x%04X):\t%s\t(%s)", cm.Name, cm.CanId, ei->Name, ElsterIndex_id, charValue, ElsterTypeStr[ei->Type]);
    //ESP_LOGI("test", "2: %s (0x%02x):\t%s(0x%04X):\t%s\t(%s)", receiver_cm.Name, receiver_cm.CanId, ei->Name, ElsterIndex_id, charValue, message_type);
    signalValue = charValue;
    return ei;
}

void readSignal(const CanMember *member, const ElsterIndex *ei)
{
    constexpr bool use_extended_id = false; // No use of extended ID
    const uint8_t IndexByte1 = static_cast<uint8_t>(ei->Index >> 8);
    const uint8_t IndexByte2 = static_cast<uint8_t>(ei->Index & 0xFF);
    std::vector<uint8_t> data;

    if (IndexByte1 == 0x00)
    {
        data = {member->ReadId[0],
                member->ReadId[1],
                IndexByte2,
                0x00,
                0x00,
                0x00,
                0x00};
    }
    else
    {
        data = {member->ReadId[0],
                member->ReadId[1],
                0xFA,
                IndexByte1,
                IndexByte2,
                0x00,
                0x00};
    }

    char logmsg[120];
    snprintf(logmsg, sizeof(logmsg), "READ \"%s\" (0x%04x) FROM %s (0x%02x {0x%03x, 0x%03x}): %02x, %02x, %02x, %02x, %02x, %02x, %02x", ei->Name, ei->Index, member->Name, member->CanId, member->ReadId[0], member->ReadId[1], data[0], data[1], data[2], data[3], data[4], data[5], data[6]);
    ESP_LOGI("readSignal()", "%s", logmsg);

    id(my_caninterface).send_data(CanMembers[cm_espclient].CanId, use_extended_id, data);
}

void writeSignal(const CanMember *member, const ElsterIndex *ei, const char *&str)
{
    bool use_extended_id = false;
    int writeValue = TranslateString(str, ei->Type);
    uint8_t IndexByte1 = static_cast<uint8_t>(ei->Index >> 8);
    uint8_t IndexByte2 = static_cast<uint8_t>(ei->Index & 0xFF);
    std::vector<uint8_t> data;

    if (IndexByte1 == 0x00)
    {
        data = {member->WriteId[0],
                member->WriteId[1],
                IndexByte2,
                static_cast<uint8_t>(writeValue >> 8),
                static_cast<uint8_t>(writeValue & 0xFF),
                0x00,
                0x00};
    }
    else
    {
        data = {member->WriteId[0],
                member->WriteId[1],
                0xfa,
                IndexByte1,
                IndexByte2,
                static_cast<uint8_t>(writeValue >> 8),
                static_cast<uint8_t>(writeValue & 0xFF)};
    }

    char logmsg[120];
    snprintf(logmsg, sizeof(logmsg), "WRITE \"%s\" (0x%04x): \"%d\" TO: %s (0x%03x {0x%03x, 0x%03x}): %02x, %02x, %02x, %02x, %02x, %02x, %02x",
             ei->Name, ei->Index, writeValue, member->Name, member->CanId, member->WriteId[0], member->WriteId[1],
             data[0], data[1], data[2], data[3], data[4], data[5], data[6]);
    ESP_LOGI("writeSignal()", "%s", logmsg);

    id(my_caninterface).send_data(CanMembers[cm_espclient].CanId, use_extended_id, data);
}

std::string formatNumber(int number, int width)
{
    std::ostringstream oss;
    oss << std::setw(width) << std::setfill('0') << number;
    return oss.str();
}

void identifyCanMembers()
{
    ESP_LOGI("identifyCanMembers()", "Identifying CAN Members...");
    for (int i = 0; i < 21; i++)
    {
        readSignal(&CanMembers[i], GetElsterIndex("WP_STATUS"));

    }


    ESP_LOGI("identifyCanMembers()", "Identified CAN Members!");
    return;
}

void processAndUpdate(uint32_t can_id, std::vector<uint8_t> msg)
{
    std::string value;
    const ElsterIndex *ei = processCanMessage(msg, can_id, value);
    // updateSensor(can_id, ei, value);
    return;
}

// void update_COP_WW()
// {
//     id(COP_WW).publish_state((id(WAERMEERTRAG_WW_SUM).state + id(WAERMEERTRAG_2WE_WW_SUM).state) / id(EL_AUFNAHMELEISTUNG_WW_SUM).state);
//     return;
// }
// void update_COP_HEIZ()
// {
//     id(COP_HEIZ).publish_state((id(WAERMEERTRAG_HEIZ_SUM).state + id(WAERMEERTRAG_2WE_HEIZ_SUM).state) / id(EL_AUFNAHMELEISTUNG_HEIZ_SUM).state);
//     return;
// }
// void update_COP_GESAMT()
// {
//     id(COP_GESAMT).publish_state((id(WAERMEERTRAG_HEIZ_SUM).state + id(WAERMEERTRAG_2WE_HEIZ_SUM).state + id(WAERMEERTRAG_WW_SUM).state + id(WAERMEERTRAG_2WE_WW_SUM).state) / (id(EL_AUFNAHMELEISTUNG_HEIZ_SUM).state + id(EL_AUFNAHMELEISTUNG_WW_SUM).state));             
//     return;
// }

#endif