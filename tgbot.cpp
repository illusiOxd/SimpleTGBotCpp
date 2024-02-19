#include <stdio.h>
#include <tgbot/tgbot.h>
#include <vector>
#include <fstream>
#include <iostream>

using namespace TgBot;
using namespace std;

class TgUser {
private:
    string name;
    int age, password, ID;

public:
    TgUser()
    {
        name = " ";
        age = -1;
        password = -1;
    }

    string GetName() const
    {
        return name;
    }
    void SetName(string newname)
    {
        this->name = newname;
    }

    int GetAge() const
    {
        return age;
    }
    void SetAge(int newage)
    {
        this->age = newage;
    }

    int GetPassword() const
    {
        return password;
    }
    void SetPassword(int newPassword)
    {
        this->password = newPassword;
    }
};

InlineKeyboardMarkup::Ptr createInlineYesNoKeyboard() {
    InlineKeyboardMarkup::Ptr keyboard(new InlineKeyboardMarkup);
    InlineKeyboardButton::Ptr buttonYes(new InlineKeyboardButton);
    InlineKeyboardButton::Ptr buttonNo(new InlineKeyboardButton);

    buttonYes->text = "Yes";
    buttonYes->callbackData = "show_password";

    buttonNo->text = "No";
    buttonNo->callbackData = "no_show_password";

    vector<InlineKeyboardButton::Ptr> row1, row2;
    row1.push_back(buttonYes);
    row2.push_back(buttonNo);

    keyboard->inlineKeyboard.push_back(row1);
    keyboard->inlineKeyboard.push_back(row2);

    return keyboard;
}

int main()
{
    Bot bot("6927127350:AAGWg0O3VpP3e0o0HNySeW81SgRbUYwmoPY");
    TgUser PersonalUser;
    bool isRegistered = false;
    bool isWaitingForName = false, isWaitingForAge = false, isWaitingForPass = false;
    TgBot::BotCommand::Ptr startCommand(new TgBot::BotCommand);
    startCommand->command = "/start";
    startCommand->description = "Start the bot";
    bot.getEvents().onCommand("start", [&bot, &isRegistered](Message::Ptr message) {
        bot.getApi().sendMessage(message->chat->id, "Hello! Welcome to the registration process. \n /info to watch your info after registration \n /links to watch my github");
        bot.getApi().sendMessage(message->chat->id, "Do you want to register? (Yes/No)");
        });

    bot.getEvents().onCommand("links", [&bot, &PersonalUser](Message::Ptr message)
        {
            TgBot::InlineKeyboardMarkup::Ptr keyboard(new TgBot::InlineKeyboardMarkup);
            TgBot::InlineKeyboardButton::Ptr button(new TgBot::InlineKeyboardButton);

            button->text = "Github";
            button->url = "https://github.com/illusiOxd";
            keyboard->inlineKeyboard.push_back({ button });

            bot.getApi().sendMessage(message->chat->id, "Click that buttons to follow the link.", false, 0, keyboard);
        });

    bot.getEvents().onAnyMessage([&bot, &PersonalUser, &isRegistered, &isWaitingForName, &isWaitingForAge, &isWaitingForPass](Message::Ptr message) {
        // Вывод информации о том, кто пишет боту
        printf("Message from: %s\n", message->from->username.c_str());
        printf("Text: %s\n", message->text.c_str());
        if (!isRegistered) {
            if (isWaitingForName) {
                // Пользователь уже ввел имя
                PersonalUser.SetName(message->text);
                isWaitingForName = false; // Пользователь ввел имя, больше не ждем
                bot.getApi().sendMessage(message->chat->id, "Type your age: ");
                isWaitingForAge = true; // Ожидаем ввод возраста
            }
            else if (isWaitingForAge) {
                // Пользователь уже ввел возраст
                try {
                    int age = stoi(message->text);
                    PersonalUser.SetAge(age);
                    isWaitingForAge = false; // Пользователь ввел возраст, больше не ждем 
                    bot.getApi().sendMessage(message->chat->id, "Type your password: ");
                    isWaitingForPass = true; // Ожидаем ввод пароля
                }
                catch (const invalid_argument& e) {
                    bot.getApi().sendMessage(message->chat->id, "Please type a valid age.");
                }
            }
            else if (isWaitingForPass) {
                // Пользователь уже ввел пароль
                try {
                    int password = stoi(message->text);
                    PersonalUser.SetPassword(password);
                    isWaitingForPass = false; // Пользователь ввел пароль, больше не ждем
                    isRegistered = true; // Регистрация завершена
                    bot.getApi().sendMessage(message->chat->id, "Registration completed.  \n /info to watch your info after registration \n /links to watch my github \n /writefile to sync your profile data");


                }
                catch (const invalid_argument& e) {
                    bot.getApi().sendMessage(message->chat->id, "Please type a valid password.");
                }
            }
            else if (message->text == "Yes" || message->text == "yes") {
                // Пользователь согласился зарегистрироваться
                bot.getApi().sendMessage(message->chat->id, "Type your name: ");
                isWaitingForName = true; // Ожидаем ввод имени
            }
        }
        else {
            // Обработка вывода информации о пользователе
            if (StringTools::startsWith(message->text, "/info")) {
                string name = PersonalUser.GetName();
                if (!name.empty()) {
                    bot.getApi().sendMessage(message->chat->id, "Your name: " + name);
                }
                else {
                    bot.getApi().sendMessage(message->chat->id, "Your name is not provided yet.");
                }
                bot.getApi().sendMessage(message->chat->id, "Your age: " + to_string(PersonalUser.GetAge()));
                bot.getApi().sendMessage(message->chat->id, "Do you want to watch your password?", false, 0,
                    createInlineYesNoKeyboard());
            }
        }
        });

    bot.getEvents().onCallbackQuery([&bot, &PersonalUser](CallbackQuery::Ptr query) {
        string data = query->data;
        string username = query->from->username;

        if (data == "show_password") {
            cout << "This user clicked 'Yes' button in password show process: " << username << endl;
            // Пользователь согласился показать пароль
            int password = PersonalUser.GetPassword();
            bot.getApi().sendMessage(query->message->chat->id, "Your password: " + to_string(password));
        }
        else if (data == "no_show_password") {
            cout << "This user clicked 'No' button in password show process: " << username << endl;
            // Пользователь отказался показывать пароль
            bot.getApi().sendMessage(query->message->chat->id, "Okay, I won't show you your password.");
        }
        });

    bot.getEvents().onCommand("writefile", [&bot, &PersonalUser](Message::Ptr message)
        {
            ifstream inFile("user_data.txt");
            bool usernameExists = false;

            // Проверяем, есть ли уже юзернейм в файле
            if (inFile.is_open()) {
                string line;
                while (getline(inFile, line)) {
                    if (line.find("TG username: " + message->from->username) != string::npos) {
                        // Если нашли юзернейм, ставим флаг и выходим из цикла
                        usernameExists = true;
                        break;
                    }
                }
                inFile.close();
            }

            if (!usernameExists) {
                // Если юзернейма еще нет в файле, производим запись
                ofstream outFile("user_data.txt", ios::app);
                if (outFile.is_open()) {
                    // Определяем последний ID из файла
                    int lastID = 0;
                    ifstream inFile("user_data.txt");
                    if (inFile.is_open()) {
                        string line;
                        while (getline(inFile, line)) {
                            if (line.find("ID:") != string::npos) {
                                int userID = stoi(line.substr(line.find(":") + 2));
                                if (userID > lastID) {
                                    lastID = userID;
                                }
                            }
                        }
                        inFile.close();
                    }

                    // Увеличиваем ID на 1, чтобы получить новый уникальный ID
                    int newID = lastID + 1;

                    // Записываем данные в файл
                    outFile << "ID: " << setfill('0') << setw(4) << newID << endl; // Форматируем ID как 4-значное число с ведущими нулями
                    outFile << "Name: " << PersonalUser.GetName() << endl;
                    outFile << "TG username: " << message->from->username << endl;
                    outFile << "Age: " << PersonalUser.GetAge() << endl;
                    outFile << "Password: " << PersonalUser.GetPassword() << endl;
                    outFile << "----------------------" << endl;
                    bot.getApi().sendMessage(message->chat->id, "Your data is written.");

                    outFile.close();
                }
            }
            else {
                bot.getApi().sendMessage(message->chat->id, "Your username is already in the file.");
            }
        });

    try
    {
        printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
        TgLongPoll longPoll(bot);
        printf("Long poll started\n");
        while (true)
        {
            longPoll.start();
        }
    }
    catch (TgException& e)
    {
        printf("error: %s\n", e.what());
    }
    return 0;
}
