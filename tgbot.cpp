#include <stdio.h>
#include <tgbot/tgbot.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <curl/curl.h>
#include <json/json.h>
#include <Windows.h>
#include <codecvt>
#include <locale>
#include <cstdio>
#include <string>
using namespace TgBot;
using namespace std;


string ConvertMessageToString(Message::Ptr message) {
    if (message != nullptr) {
        return message->text;
    }
    else {
        return ""; 
    }
}



class TgUser {
private:
    string name, password;
    int age, ID;
    bool isRegistered;
    bool waitingForCity;

public: 
    int NoteCount; 
public:
    TgUser() {
        name = "";
        age = -1;
        password = "";
        isRegistered = false;
        waitingForCity = false;
        NoteCount = 1;
    }

    string GetName() const {
        return name;
    }
    void SetName(string newname) {
        this->name = newname;
    }

    int GetAge() const {
        return age;
    }
    void SetAge(int newage) {
        this->age = newage;
    }

    string GetPassword() const {
        return password;
    }
    void SetPassword(string newPassword) {
        this->password = newPassword;
    }
    bool IsRegistered() const {
        return isRegistered;
    }

    bool IsWaitingForCity() const {
        return waitingForCity;
    }

    void SetWaitingForCity(bool isWaiting) {
        waitingForCity = isWaiting;
    }

    wstring ConvertToWString(const string& str) {
        wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;
        return converter.from_bytes(str);
    }

    void sendMessageWithoutKeyboard(int64_t chatId, const string& text, Bot& bot) {
        bot.getApi().sendMessage(chatId, text, false, 0, nullptr);
    }

    vector<string> GetNotes(int userId, const string& username) {
        vector<string> notes;
        string filename = "user_notes_" + to_string(userId) + "_" + username + "_data.txt";
        ifstream inFile(filename);

        if (!inFile.is_open()) {
            cout << "File " << filename << " not found or cannot be opened." << endl;
            return notes;
        }

        string line;
        while (getline(inFile, line)) {
            if (line.find("Note number ") == 0) {
                // Предполагается, что каждая заметка начинается с "Note number "
                // Добавляем заметку в вектор
                notes.push_back(line.substr(12)); // Получаем только текст заметки
            }
        }

        inFile.close();
        return notes;
    }

    void FileCheck(ifstream& inFile, string filename, map<int, TgUser>& userMap, Bot& bot, Message::Ptr message, int userId, ReplyKeyboardMarkup::Ptr(*createBottomKeyboard)())
    {
        if (inFile.is_open()) {
            isRegistered = true;
            string line;
            while (getline(inFile, line)) {
                if (line.find("Name: ") != string::npos) {
                    userMap[userId].SetName(line.substr(6));
                }
                else if (line.find("Age: ") != string::npos) {
                    userMap[userId].SetAge(stoi(line.substr(5)));
                }
                else if (line.find("Password: ") != string::npos) {
                    userMap[userId].SetPassword(line.substr(10));
                }
            }

            inFile.close();

            if (userMap[userId].GetName() != " " && userMap[userId].GetAge() != -1 && userMap[userId].GetPassword() != " - 1") {
                wstring utf16String = L"Ваша дата загружена.";
                bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF8(utf16String), false, 0, createBottomKeyboard());
            }
            else {
                wstring utf16String = L"Ваша дата не загружена.";
                bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF8(utf16String), false, 0, nullptr);
            }
        }
        else {
            if (!isRegistered) {
                wstring utf16String = L"Хотите зарегистрироваться?(Да/Нет)";
                bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF8(utf16String), false, 0, nullptr);
            }
            else {
                wstring utf16String = L"Вы уже зарегистрированы.";
                bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF8(utf16String), false, 0, createBottomKeyboard());
            }
            return;
        }
    }

    void FileWriteCheck(ifstream& inFile, int userId, string filename, bool usernameExists, map<int, TgUser>& userMap, Bot& bot, Message::Ptr message, ReplyKeyboardMarkup::Ptr(*createBottomKeyboard)())
    {
        usernameExists = false;

        if (inFile.is_open()) {
            string line;
            while (getline(inFile, line)) {
                if (line.find("TG username: " + message->from->username) != string::npos) {
                    usernameExists = true;
                    break;
                }
            }
            inFile.close();
        }

        if (!usernameExists) {
            ofstream outFile(filename, ios::app);

            if (outFile.is_open()) {
                outFile << "Name: " << userMap[userId].GetName() << endl;
                outFile << "TG username: " << message->from->username << endl;
                outFile << "Age: " << userMap[userId].GetAge() << endl;
                outFile << "Password: " << userMap[userId].GetPassword() << endl;
                outFile << "----------------------" << endl;

                wstring utf16String = L"Ваша дата записана.";
                bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF8(utf16String), false, 0, createBottomKeyboard());
                outFile.close();
            }
        }
        else {
            wstring utf16String = L"Ваш никнейм телеграмм уже в файле.";
            bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF8(utf16String), false, 0, createBottomKeyboard());
        }
    }

    void LoginCheck(int userId, Message::Ptr message, map<int, TgUser>& userMap, Bot& bot, bool& isRegistered, bool& isWaitingForName, bool& isWaitingForAge, bool& isWaitingForPass, ReplyKeyboardMarkup::Ptr(*createBottomKeyboard)(), InlineKeyboardMarkup::Ptr(*createInlineYesNoKeyboard)(), ifstream& inFile, string filename, bool usernameExists)
    {
        if (!isRegistered) {
            if (isWaitingForName) {
                userMap[userId].SetName(message->text);
                isWaitingForName = false;
                wstring utf16String = L"Введите свой возраст: ";
                bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF8(utf16String), false, 0);
                isWaitingForAge = true;
            }
            else if (isWaitingForAge) {
                try {
                    int age = stoi(message->text);
                    userMap[userId].SetAge(age);
                    isWaitingForAge = false;
                    wstring utf16String = L"Введите свой пароль: ";
                    bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF8(utf16String), false, 0);
                    isWaitingForPass = true;
                }
                catch (const invalid_argument& e) {
                    wstring utf16String = L"Пожалуйста, введите правильный возраст: ";
                    bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF8(utf16String), false, 0);
                }
            }
            else if (isWaitingForPass) {
                try {
                    string password = message->text;
                    userMap[userId].SetPassword(password);
                    isWaitingForPass = false;
                    isRegistered = true;
                    FileWriteCheck(inFile, userId, filename, usernameExists, userMap, bot, message, createBottomKeyboard);
                    // FileWriteCheck(ifstream & inFile, int userId, string filename, bool usernameExists, map<int, TgUser>&userMap, Bot & bot, Message::Ptr message, ReplyKeyboardMarkup::Ptr(*createBottomKeyboard)())
                    wstring utf16String = L"Регистрация завершена.";
                    bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF8(utf16String), false, 0, createBottomKeyboard());
                }
                catch (const invalid_argument& e) {
                    wstring utf16String = L"Пожалуйста, введите правильный пароль: ";
                    bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF8(utf16String), false, 0);
                }
            }
            else if (ConvertToWString(message->text) == L"Да" || ConvertToWString(message->text) == L"да") {
                wstring utf16String = L"Введите своё имя: ";
                bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF8(utf16String), false, 0);
                isWaitingForName = true;
            }
        }
    }
    
    void InfoAboutUser(InlineKeyboardMarkup::Ptr(*createInlineYesNoKeyboard)(), int userId, Message::Ptr message, map<int, TgUser>& userMap, Bot& bot, ReplyKeyboardMarkup::Ptr(*createBottomKeyboard)())
    {
        if (!name.empty()) {
            wstring utf16String = L"Ваше имя: ";
            bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF8(utf16String) + name, false, 0, createBottomKeyboard());
        }
        else {
            wstring utf16String = L"Ваше имя не указано. ";
            bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF8(utf16String), false, 0, createBottomKeyboard());
        }

        wstring utf16String3 = L"Ваш возраст: ";
        wstring utf16String4 = L"Хотите увидеть ваш пароль? ";
        bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF8(utf16String3) + to_string(userMap[userId].GetAge()), false, 0, createBottomKeyboard());
        bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF8(utf16String4), false, 0, createInlineYesNoKeyboard());
    }

    void DataChoice(string callbackData, map<int, TgUser>& userMap, int userId, Bot& bot, CallbackQuery::Ptr query, ReplyKeyboardMarkup::Ptr(*createBottomKeyboard)())
    {
        if (callbackData == "show_password") {
            string password = userMap[userId].GetPassword();
            wstring utf16String = L"Ваш пароль: ";
            bot.getApi().sendMessage(query->message->chat->id, ConvertUTF16toUTF8(utf16String) + password, false, 0, createBottomKeyboard());
            cout << "Пользователь нажал кнопку 'Да' в выводе пароля. " << endl;
        }
        else if (callbackData == "no_show_password") {
            wstring utf16String = L"Вы выбрали не показывать ваш пароль.";
            bot.getApi().sendMessage(query->message->chat->id, ConvertUTF16toUTF8(utf16String), false, 0, createBottomKeyboard());
            cout << "Пользователь нажал кнопку 'Нет' в выводе пароля. " << endl;
        }
    }

    void CreateLinkButton(Bot& bot, Message::Ptr message)
    {
        InlineKeyboardMarkup::Ptr keyboard(new InlineKeyboardMarkup);
        InlineKeyboardButton::Ptr button(new InlineKeyboardButton);

        button->text = "Github";
        button->url = "https://github.com/illusiOxd";
        keyboard->inlineKeyboard.push_back({ button });

        wstring utf16String = L"Нажмите кнопку, что бы увидеть ссылку на мои соц. сети: ";
        bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF8(utf16String), false, 0, keyboard);
    }

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* buffer) {
        // Рассчитываем общий размер полученных данных, умножая размер каждого элемента на количество элементов.
        size_t totalSize = size * nmemb;
        // Преобразуем полученные данные (буфер) в строку и добавляем к уже имеющимся данным в буфере.
        buffer->append((char*)contents, totalSize);
        // Возвращаем общий размер полученных данных.
        return totalSize;
    }

    // Ваша функция GetWeather
    string GetWeather(string& city, string& apiKey_) {
        setlocale(LC_ALL, "Russian");
        // Инициализация библиотеки libcurl для работы с HTTP запросами.
        CURL* curl = curl_easy_init();
        // Проверка успешной инициализации CURL.
        if (!curl) {
            return "Failed to initialize CURL";
        }

        // Формирование URL-адреса для отправки запроса на получение данных о погоде для указанного города с использованием указанного API ключа.
        string url = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "&appid=" + apiKey_;
        // Создание строки для хранения полученных данных.
        string readBuffer;

        // Установка параметров CURL для выполнения HTTP запроса.
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // Устанавливаем URL для запроса.
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback); // Устанавливаем функцию обратного вызова для записи данных.
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer); // Устанавливаем буфер, в который будут записываться полученные данные.

        // Выполнение HTTP запроса.
        CURLcode res = curl_easy_perform(curl);
        // Освобождение ресурсов, связанных с CURL.
        curl_easy_cleanup(curl);

        // Проверка результата выполнения запроса.
        if (res != CURLE_OK) {
            // В случае ошибки возвращаем сообщение об ошибке.
            return "Failed to fetch weather data: " + string(curl_easy_strerror(res));
        }

        // Парсинг полученных данных в формате JSON.
        Json::Value root;
        Json::Reader reader;
        bool parsingSuccessful = reader.parse(readBuffer, root);
        // Проверка успешности парсинга JSON.
        if (!parsingSuccessful) {
            // В случае неудачного парсинга возвращаем сообщение об ошибке.
            return "Failed to parse JSON";
        }

        // Извлечение информации о погоде из полученных данных.
        string weatherDescription = root["weather"][0]["description"].asString(); // Описание погоды.
        double temperature = root["main"]["temp"].asDouble() - 273.15; // Температура (переводим из Кельвинов в Цельсии).
        double humidity = root["main"]["humidity"].asDouble(); // Влажность.

        wstring utf16String = L"Погода в городе ";
        wstring utf16String2 = L"Температура: ";
        wstring utf16String3 = L"Влажность: ";
        wstring utf16String4 = L" градусов. ";
        wstring utf16String5 = L" процентов. ";
        int roundedHumidity = round(humidity);
        int roundedTemp = round(temperature);
        // Формирование строки с информацией о погоде для возврата.
        return ConvertUTF16toUTF8(utf16String) + city + ": " + weatherDescription + "\n" + ConvertUTF16toUTF8(utf16String2) + to_string(roundedTemp) + ConvertUTF16toUTF8(utf16String4) + "\n" + ConvertUTF16toUTF8(utf16String3) + to_string(roundedHumidity) + ConvertUTF16toUTF8(utf16String5);
    }

    string ConvertUTF16toUTF8(const wstring& utf16String) {
        wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;
        return converter.to_bytes(utf16String);
    }

    void ClearAllNotes(const char* filename, Bot& bot, Message::Ptr message, std::vector<string>& usernotes) {
        int userId = message->from->id;

        ofstream outFile(filename);

        if (outFile.is_open()) {
            wstring utf16String = L"Все записки были успешно удалены.";
            bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF8(utf16String), false, 0);
            outFile.clear();
            outFile.close();

            // Удаляем файл
            remove(filename);

            // Очищаем вектор заметок пользователя
            usernotes.clear(); // Очищаем вектор заметок
        }
        else {
            wstring utf16ErrorString = L"У вас нету записок.";
            bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF8(utf16ErrorString), false, 0);
        }
    }



    void UserNotes(ifstream& inFile, const string& filename, map<int, TgUser>& userMap, Bot& bot, Message::Ptr message, bool& isRegistered)
    {
        ofstream outFile;

        int nextNoteNumber = 1; // Начальный номер для записи заметки

        // Проверяем, существует ли файл
        inFile.open(filename);
        if (inFile) {
            // Находим последний номер заметки в файле
            string line;
            while (getline(inFile, line)) {
                if (line.find("Number: ") != string::npos) {
                    int noteNumber = stoi(line.substr(8)); // Получаем номер заметки из строки
                    nextNoteNumber = max(nextNoteNumber, noteNumber + 1); // Обновляем следующий номер
                }
            }
            inFile.close();
            outFile.open(filename, ios::app); // Открываем файл для дозаписи
        }
        else {
            // Если файл не существует, открываем его для записи
            outFile.open(filename);
            outFile << "TG Username: " << message->from->username << endl;
            outFile << "Notes" << endl;
        }

        if (outFile.is_open()) {
            outFile << "Number: " << nextNoteNumber << endl; // Записываем следующий номер заметки

            // Проверяем, не является ли введенная пользователем заметка командой /notes
            string userNote = message->text;
            if (userNote != "/notes" && userNote.length() <= 150 && !userNote.empty() && userNote[0] != '/') {
                outFile << userNote << endl;
            }
            else if (userNote != "/notes") {
                for (size_t i = 0; i < userNote.length(); i += 150) {
                    outFile << userNote.substr(i, 150) << endl;
                }
            }
            else {
                wstring utf16ErrorString = L"Ошибка: введена некорректная заметка.";
                bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF8(utf16ErrorString), false, 0);
            }

            outFile.close();
        }
        else {
            wstring utf16ErrorString = L"Ошибка при открытии файла для записи заметок.";
            bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF8(utf16ErrorString), false, 0);
        }
    }
};

string ConvertUTF16toUTF82(const wstring& utf16String) {
    wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(utf16String);
}

InlineKeyboardMarkup::Ptr createInlineYesNoKeyboard() {
    InlineKeyboardMarkup::Ptr keyboard(new InlineKeyboardMarkup);
    InlineKeyboardButton::Ptr buttonYes(new InlineKeyboardButton);
    InlineKeyboardButton::Ptr buttonNo(new InlineKeyboardButton);

    wstring utf16String = L"Да";
    buttonYes->text = ConvertUTF16toUTF82(utf16String);
    buttonYes->callbackData = "show_password";

    wstring utf16String2 = L"Нет";

    buttonNo->text = ConvertUTF16toUTF82(utf16String2);
    buttonNo->callbackData = "no_show_password";

    vector<InlineKeyboardButton::Ptr> row1, row2;
    row1.push_back(buttonYes);
    row2.push_back(buttonNo);

    keyboard->inlineKeyboard.push_back(row1);
    keyboard->inlineKeyboard.push_back(row2);

    return keyboard;
}

map<int, TgUser> usersData;

bool containsCyrillic(const wstring& str) {
    for (char c : str) {
        if (c >= 0xC0 && c <= 0xFF) {
            return true;
        }
    }
    return false;
}

bool containsLatin(const string& str) {
    for (char c : str) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
            return true;
        }
    }
    return false;
}

ReplyKeyboardMarkup::Ptr createBottomKeyboard() {
    ReplyKeyboardMarkup::Ptr keyboard(new ReplyKeyboardMarkup);
    vector<KeyboardButton::Ptr> row1, row2;

    KeyboardButton::Ptr button1(new KeyboardButton);
    button1->text = "/start";
    row1.push_back(button1);

    KeyboardButton::Ptr button2(new KeyboardButton);
    button2->text = "/info";
    row1.push_back(button2);

    KeyboardButton::Ptr button3(new KeyboardButton);
    button3->text = "/notes";
    row1.push_back(button3);

    KeyboardButton::Ptr button7(new KeyboardButton);
    button7->text = "/shownotes";
    row1.push_back(button7);

    KeyboardButton::Ptr button4(new KeyboardButton);
    button4->text = "/links";
    row2.push_back(button4);

    KeyboardButton::Ptr button5(new KeyboardButton);
    button5->text = "/weather";
    row2.push_back(button5);

    KeyboardButton::Ptr button6(new KeyboardButton);
    button6->text = "/noteswrite";
    row2.push_back(button6);

    //KeyboardButton::Ptr button8(new KeyboardButton);
    //button8->text = "/notesdelete";
    //row2.push_back(button8);

    keyboard->keyboard.push_back(row1);
    keyboard->keyboard.push_back(row2);

    keyboard->resizeKeyboard = true;
    keyboard->oneTimeKeyboard = false;
    keyboard->selective = false;

    return keyboard;
}

void InfoMenu(InlineKeyboardMarkup::Ptr(*createInlineYesNoKeyboard)(), int userId, Message::Ptr message, map<int, TgUser>& userMap, Bot& bot, ReplyKeyboardMarkup::Ptr(*createBottomKeyboard)()) {
    string name = userMap[userId].GetName();
    int age = userMap[userId].GetAge();
    string filename = "user_notes_" + to_string(userId) + "_" + name + "_data.txt";
    vector<string> notes = userMap[userId].GetNotes(userId, filename);

    // Отображаем информацию о пользователе
    if (!name.empty()) {
        wstring utf16String = L"Ваше имя: ";
        bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF82(utf16String) + name, false, 0, createBottomKeyboard());
    }
    else {
        wstring utf16String = L"Ваше имя не указано. ";
        bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF82(utf16String), false, 0, createBottomKeyboard());
    }

    wstring utf16String = L"Ваш возраст: ";
    bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF82(utf16String) + to_string(age), false, 0, createBottomKeyboard());

    // Отображаем заметки пользователя
    if (notes.empty()) {
        wstring utf16NoNotes = L"У вас нету текущих заметок.";
        bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF82(utf16NoNotes), false, 0, createBottomKeyboard());
    }
    else {
        wstring utf16NotesHeader = L"Ваши заметки:";
        bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF82(utf16NotesHeader), false, 0, createBottomKeyboard());

        for (const auto& note : notes) {
            bot.getApi().sendMessage(message->chat->id, note, false, 0, createBottomKeyboard());
        }
    }
}

int main() {
    vector <string> usernotes; // Вектор для хранения заметок
    setlocale(LC_ALL, "ru_RU.utf8");
    Bot bot("6927127350:AAGWg0O3VpP3e0o0HNySeW81SgRbUYwmoPY");
    map<int, TgUser> userMap;
    map<int, bool> awaitingNote;
    bool isRegistered = false;
    bool isWaitingForName = false, isWaitingForAge = false, isWaitingForPass = false;
    string CurrentNote;

    bot.getEvents().onCommand("start", [&bot, &isRegistered, &userMap](Message::Ptr message) {
        int userId = message->from->id;


        string filename = "user_" + to_string(userId) + "_" + message->from->username.c_str() + "_data.txt";
        ifstream inFile(filename);

        if (userMap[userId].IsRegistered()) {
            wstring utf16String = L"Вы уже зарегистрированы.";
            bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF82(utf16String), false, 0, createBottomKeyboard());
            return;
        }

        wstring utf16String = L"Главное меню. \n/start - главное меню \n/info - кабинет пользователя \n/links - ссылки на мои соц. сети \n/weather - прогноз погоды на выбранный город \n/shownotes - показать ваши нотатки \n/notes - записать вашу нотатку \n/noteswrite - записать вашу нотатку, которую вы ввели";
        bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF82(utf16String), false, 0, createBottomKeyboard());

        userMap[userId].FileCheck(inFile, filename, userMap, bot, message, userId, &createBottomKeyboard);
        });


    bot.getEvents().onAnyMessage([&bot, &userMap, &isRegistered, &isWaitingForName, &isWaitingForAge, &isWaitingForPass, &usernotes, &awaitingNote, &CurrentNote](Message::Ptr message) {
        printf("Message from: %s\n", message->from->username.c_str());
        printf("Text: %s\n", message->text.c_str());

        int userId = message->from->id;
        string filename = "user_" + to_string(userId) + "_" + message->from->username.c_str() + "_data.txt";
        ifstream inFile(filename);
        bool usernameExists = false;

        if (message->text.find("/notes") == 0) {
            // Обработка команды /notes
            awaitingNote[userId] = true;
        }
        else if (awaitingNote.find(userId) != awaitingNote.end() && awaitingNote[userId]) {
            // Обработка пользовательской заметки
            usernotes.push_back(message->text);
            CurrentNote = message->text;
            wstring utf16Message2 = L"Ваша заметка сохранена.";
            bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF82(utf16Message2), false, 0);
            wstring utf16Message3 = L"Ваша заметка: ";
            bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF82(utf16Message3), false, 0);
            bot.getApi().sendMessage(message->chat->id, usernotes.back(), false, 0);
            awaitingNote[userId] = false; // Сбрасываем флаг ожидания заметки
        }
        else if (userMap[userId].IsWaitingForCity()) {
            // Обработка ожидания города для погоды
            string city = message->text;
            if (!city.empty()) {
                string apiKey = "9412019bd8271cee3f3d2c0c5ace2248";
                string weatherInfo = userMap[userId].GetWeather(city, apiKey);
                bot.getApi().sendMessage(message->chat->id, weatherInfo, false, 0, createBottomKeyboard());
            }
            else {
                wstring utf16String = L"Город не может быть пустым.";
                bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF82(utf16String), false, 0, createBottomKeyboard());
            }
            userMap[userId].SetWaitingForCity(false);
        }
        else {
            userMap[userId].LoginCheck(userId, message, userMap, bot, isRegistered, isWaitingForName, isWaitingForAge, isWaitingForPass, &createBottomKeyboard, &createInlineYesNoKeyboard, inFile, filename, usernameExists);
        }
        });

    bot.getEvents().onCommand("info", [&bot, &userMap](Message::Ptr message) { // инфа о юзере
        int userId = message->from->id;

        if (userMap[userId].IsRegistered())
        {
            userMap[userId].InfoAboutUser(&createInlineYesNoKeyboard, userId, message, userMap, bot, &createBottomKeyboard);
        }
        else
        {
            wstring utf16ErrorString = L"Сначала зарегистрируйтесь!";
            bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF82(utf16ErrorString), false, 0);
        }

        //  InfoAboutUser(InlineKeyboardMarkup::Ptr(*createInlineYesNoKeyboard)(), int userId, Message::Ptr message, map<int, TgUser>& userMap, Bot& bot, ReplyKeyboardMarkup::Ptr(*createBottomKeyboard)())
        });

    bot.getEvents().onCommand("weather", [&bot, &userMap](Message::Ptr message) { // погода
        int userId = message->from->id;
        if (userMap[userId].IsRegistered())
        {
            wstring utf16String = L"Введите свой город: ";
            bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF82(utf16String), false, 0, createBottomKeyboard());
            userMap[userId].SetWaitingForCity(true);
        }
        else
        {
            wstring utf16ErrorString = L"Сначала зарегистрируйтесь!";
            bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF82(utf16ErrorString), false, 0);
        }
        });

    bot.getEvents().onCommand("links", [&bot, &userMap](Message::Ptr message) { // ссылки 
        int userId = message->from->id;
        // CreateLinkButton(Bot& bot, Message::Ptr message)
        userMap[userId].CreateLinkButton(bot, message);
        });

    bot.getEvents().onCallbackQuery([&bot, &userMap](CallbackQuery::Ptr query) { // действия юзера с интерактивными элементами 
        int userId = query->from->id;
        string callbackData = query->data;

        // DataChoice(string callbackData, map<int, TgUser>& userMap, int userId, Bot& bot, CallbackQuery::Ptr query)
        userMap[userId].DataChoice(callbackData, userMap, userId, bot, query, &createBottomKeyboard);
        });

    //bot.getEvents().onCommand("notesdelete", [&bot, &userMap, &usernotes](Message::Ptr message) {
    //    int userId = message->from->id;
    //    string username = message->from->username;
    //    const char* filename2 = username.c_str();
    //    const string filename = "user_notes_" + to_string(userId) + "_" + username + "_data.txt";
    //    userMap[userId].ClearAllNotes(filename2, bot, message, usernotes);
    //    });

    bot.getEvents().onCommand("noteswrite", [&bot, &userMap, &isRegistered, &usernotes, &awaitingNote, &CurrentNote](Message::Ptr message) {
        int userId = message->from->id;
        string username = message->from->username;
        string filename = "user_notes_" + to_string(userId) + "_" + username + "_data.txt";

        // Проверяем существование файла и создаем его, если он не существует
        ofstream outFile(filename, ios::app);
        if (!outFile.is_open()) {
            wstring utf16Message3 = L"Ошибка открытия файла для записи заметок. ";
            bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF82(utf16Message3), false, 0);
            return;
        }
        outFile.close(); // Закрываем файл

        // Открываем файл для чтения
        ifstream inFile(filename);
        if (!inFile.is_open()) {
            wstring utf16Message3 = L"Ошибка открытия файла для записи заметок. ";
            bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF82(utf16Message3), false, 0);
            return;
        }

        int lastNoteNumber = 0;
        string line;
        while (getline(inFile, line)) {
            if (line.find("Note number ") == 0) { // Ищем строку, начинающуюся с "Note number "
                int noteNumber = stoi(line.substr(12)); // Получаем номер заметки из строки
                lastNoteNumber = max(lastNoteNumber, noteNumber); // Обновляем последний номер заметки
            }
        }

        inFile.close();

        // Открываем файл для добавления записей в конец файла
        ofstream outFileAppend(filename, ios::app);
        if (!outFileAppend.is_open()) {
            wstring utf16Message3 = L"Ошибка открытия файла для записи заметок. ";
            bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF82(utf16Message3), false, 0);
            return;
        }

        // Записываем заметки из currentnote в файл и в вектор usernotes
        outFileAppend << "Note number " << lastNoteNumber + 1 << endl; // Записываем новую заметку с номером последний + 1
        outFileAppend << CurrentNote << endl;
        usernotes.push_back(CurrentNote);
        lastNoteNumber++;

        wstring utf16Message3 = L"Заметки успешно записаны в файл. ";
        bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF82(utf16Message3), false, 0);
        outFileAppend.close();
        awaitingNote[userId] = false;
        });

    bot.getEvents().onCommand("notes", [&bot, &userMap, &isRegistered, &usernotes, &awaitingNote](Message::Ptr message) {
        int userId = message->from->id;
        wstring utf16Message = L"Введите свою заметку.";
        bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF82(utf16Message), false, 0);
        // Устанавливаем флаг, что пользователь ожидает ввода заметки
        awaitingNote[userId] = true;

        });

    bot.getEvents().onCommand("shownotes", [&bot, &userMap](Message::Ptr message) {
        int userId = message->from->id;
        string username = message->from->username;
        string filename = "user_notes_" + to_string(userId) + "_" + username + "_data.txt";

        ifstream inFile(filename); // Открываем файл для чтения

        if (!inFile.is_open()) {
            wstring utf16ErrorString = L"У вас нет текущих заметок.";
            bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF82(utf16ErrorString), false, 0);
        }
        else {
            string line;
            while (getline(inFile, line)) {
                if (line.find("Note number ") == 0) { // Выводим номер заметки
                    bot.getApi().sendMessage(message->chat->id, line, false, 0);
                }
                else { // Выводим содержимое заметки
                    bot.getApi().sendMessage(message->chat->id, line, false, 0);
                }
            }
            inFile.close();
        }
        // Добавим логику создания файла, если его нет
        ofstream outFile(filename, std::ios::app);
        if (!outFile.is_open()) {
            wstring utf16ErrorString = L"Ошибка открытия файла для записи заметок.";
            bot.getApi().sendMessage(message->chat->id, ConvertUTF16toUTF82(utf16ErrorString), false, 0);
        }
        });

    try {
        printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
        TgLongPoll longPoll(bot);
        printf("Long poll started\n");
        while (true) {
            longPoll.start();
        }
    }
        catch (TgException& e) {
            printf("error: %s\n", e.what());
        }
        return 0;
    }
