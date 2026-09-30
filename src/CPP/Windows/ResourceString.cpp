// Adapted from 7-Zip CPP/Windows/ResourceString.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Win32 LoadString/HINSTANCE storage is replaced by compiled source STRINGTABLE
// strings and the existing fixed Russian UI captions. Language files: P1-14.
#include "ResourceString.h"
#include "7zip/UI/FileManager/OverwriteDialogRes.h"
#include "7zip/UI/FileManager/ProgressDialog2Res.h"
#include "7zip/UI/FileManager/PropertyNameRes.h"
#include "7zip/UI/FileManager/resource.h"
#include "7zip/UI/FileManager/resourceGui.h"
#include "7zip/UI/GUI/ExtractRes.h"
#include "7zip/UI/GUI/resource3.h"
#include "CPP/7zip/UI/FileManager/MessagesDialogRes.h"

namespace NWindows {

struct ResourceStringEntry {
    UINT Id;
    const wchar_t *Text;
};

#include "ResourceStrings.h"

static const ResourceStringEntry kLocalizedStrings[] = {
    {IDS_MESSAGE, L"Сообщение"},
    {IDS_MESSAGE_NO_ERRORS, L"Ошибок нет"},
    {IDS_PROGRESS_TESTING, L"Проверка"},
    {IDS_MOVING, L"Перемещение"},
    {IDS_CHECKSUM_CALCULATING, L"Расчёт контрольных сумм"},
    {IDS_CHECKSUM_INFORMATION, L"Контрольные суммы"},
    {IDS_CHECKSUM_CRC_DATA, L"Контрольная сумма данных:"},
    {IDS_CHECKSUM_CRC_DATA_NAMES, L"Контрольная сумма данных и имён:"},
    {IDS_CHECKSUM_CRC_STREAMS_NAMES, L"Контрольная сумма потоков и имён:"},
    {IDS_OPENNING, L"Открытие"},
    {IDS_SCANNING, L"Сканирование"},
    {IDS_FILE_SIZE, L"{0} байт"},
    {IDS_MEM_REQUIRES_BIG_MEM, L"Для операции требуется большой объём памяти."},
    {IDS_MEM_REQUIRED_MEM_SIZE, L"Требуется памяти"},
    {IDS_MEM_CURRENT_MEM_LIMIT, L"Текущий лимит памяти"},
    {IDS_MEM_RAM_SIZE, L"Оперативная память"},
    {IDS_PROGRESS_PAUSED, L"Приостановлено"},
    {IDS_PROGRESS_FOREGROUND, L"На &передний план"},
    {IDS_CONTINUE, L"&Продолжить"},
    {IDS_PROGRESS_ASK_CANCEL, L"Отменить операцию?"},
    {IDS_CLOSE, L"&Закрыть"},
    {IDS_PROGRESS_REMOVE, L"Удаление"},
    {IDS_PROGRESS_ADD, L"Добавление"},
    {IDS_PROGRESS_UPDATE, L"Обновление"},
    {IDS_PROGRESS_ANALYZE, L"Анализ"},
    {IDS_PROGRESS_REPLICATE, L"Репликация"},
    {IDS_PROGRESS_REPACK, L"Перепаковка"},
    {IDS_PROGRESS_DELETE, L"Удаление"},
    {IDS_PROGRESS_HEADER, L"Создание заголовка"},
    {IDS_WANT_UPDATE_MODIFIED_FILE, L"Файл '{0}' был изменён.\nОбновить его в архиве?"},
    {IDS_CANNOT_UPDATE_FILE, L"Не удалось обновить файл\n'{0}'"},
    {IDS_PROP_PATH, L"Путь"},
    {IDS_PROP_NAME, L"Имя"},
    {IDS_PROP_EXTENSION, L"Расширение"},
    {IDS_PROP_IS_FOLDER, L"Папка"},
    {IDS_PROP_SIZE, L"Размер"},
    {IDS_PROP_PACKED_SIZE, L"Сжатый размер"},
    {IDS_PROP_ATTRIBUTES, L"Атрибуты"},
    {IDS_PROP_CTIME, L"Создан"},
    {IDS_PROP_ATIME, L"Открыт"},
    {IDS_PROP_MTIME, L"Изменён"},
    {IDS_PROP_ENCRYPTED, L"Зашифрован"},
    {IDS_PROP_METHOD, L"Метод"},
    {IDS_PROP_USER, L"Пользователь"},
    {IDS_PROP_GROUP, L"Группа"},
    {IDS_PROP_COMMENT, L"Комментарий"},
    {IDS_PROP_PREFIX, L"Префикс пути"},
    {IDS_PROP_FOLDERS, L"Папок"},
    {IDS_PROP_FILES, L"Файлов"},
    {IDS_PROP_POSIX_ATTRIB, L"Режим"},
    {IDS_PROP_SYM_LINK, L"Символическая ссылка"},
    {IDS_PROP_ERROR, L"Ошибка"},
    {IDS_PROP_ERROR_TYPE, L"Тип ошибки"},
    {IDS_PROP_NUM_ERRORS, L"Ошибок"},
    {IDS_PROP_ERROR_FLAGS, L"Ошибки"},
    {IDS_PROP_WARNING_FLAGS, L"Предупреждения"},
    {IDS_PROP_WARNING, L"Предупреждение"},
    {IDS_EXTRACT_MSG_IS_NOT_ARC, L"Не является архивом"},
    {IDS_EXTRACT_MSG_HEADERS_ERROR, L"Ошибка заголовков"},
    {IDS_OPEN_MSG_UNAVAILABLE_START, L"Недоступно начало архива"},
    {IDS_OPEN_MSG_UNCONFIRMED_START, L"Начало архива не подтверждено"},
    {IDS_EXTRACT_MSG_UEXPECTED_END, L"Неожиданный конец данных"},
    {IDS_EXTRACT_MSG_DATA_AFTER_END, L"Есть данные после конца содержимого"},
    {IDS_EXTRACT_MSG_UNSUPPORTED_METHOD, L"Неподдерживаемый метод сжатия"},
    {IDS_OPEN_MSG_UNSUPPORTED_FEATURE, L"Неподдерживаемая функция"},
    {IDS_EXTRACT_MSG_DATA_ERROR, L"Ошибка данных"},
    {IDS_EXTRACT_MSG_CRC_ERROR, L"Ошибка CRC"},
    {IDS_EXTRACT_MSG_WRONG_PSW_GUESS, L"Неверный пароль?"},
};

UString MyLoadString(UINT resourceID) {
    for (const auto &entry : kLocalizedStrings)
        if (entry.Id == resourceID)
            return entry.Text;
    for (const auto &entry : kResourceStrings)
        if (entry.Id == resourceID)
            return entry.Text;
    return UString();
}

void MyLoadString(UINT resourceID, UString &dest) { dest = MyLoadString(resourceID); }

#ifdef _WIN32
void MyLoadString(HINSTANCE, UINT resourceID, UString &dest) { MyLoadString(resourceID, dest); }
#endif

} // namespace NWindows
