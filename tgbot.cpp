#include <stdio.h>
#include <tgbot/tgbot.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <map>

using namespace TgBot;
using namespace std;

class TgUser {
private:
    string name;
    int age, password, ID;
    bool isRegistered;

public:
    TgUser() {
        name = " ";
        age = -1;
        password = -1;
        isRegistered = false;
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

    int GetPassword() const {
        return password;
    }
    void SetPassword(int newPassword) {
        this->password = newPassword;
    }
    bool IsRegistered() const {
        return isRegistered;
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
    button3->text = "/links";
    row2.push_back(button3);

    KeyboardButton::Ptr button4(new KeyboardButton);
    button4->text = "/writefile";
    row2.push_back(button4);

    keyboard->keyboard.push_back(row1);
    keyboard->keyboard.push_back(row2);

    keyboard->resizeKeyboard = true;
    keyboard->oneTimeKeyboard = false;
    keyboard->selective = false;

    return keyboard;
}

map<int, TgUser> usersData;

int main() {
    Bot bot("6927127350:AAGWg0O3VpP3e0o0HNySeW81SgRbUYwmoPY");
    map<int, TgUser> userMap;

    bool isRegistered = false;
    bool isWaitingForName = false, isWaitingForAge = false, isWaitingForPass = false;

    bot.getEvents().onCommand("start", [&bot, &isRegistered, &userMap](Message::Ptr message) {
        int userId = message->from->id;

        TgUser PersonalUser;

        string filename = "user_" + to_string(userId) + "_" + message->from->username.c_str() + "_data.txt";
        ifstream inFile(filename);

        if (inFile.is_open()) {
            isRegistered = true;
            string line;
            while (getline(inFile, line)) {
                if (line.find("Name: ") != string::npos) {
                    PersonalUser.SetName(line.substr(6));
                }
                else if (line.find("Age: ") != string::npos) {
                    PersonalUser.SetAge(stoi(line.substr(5)));
                }
                else if (line.find("Password: ") != string::npos) {
                    PersonalUser.SetPassword(stoi(line.substr(10)));
                }
            }

            userMap[userId] = PersonalUser;

            inFile.close();

            // Проверяем наличие данных перед отправкой сообщения
            if (userMap[userId].GetName() != " " && userMap[userId].GetAge() != -1 && userMap[userId].GetPassword() != -1) {
                bot.getApi().sendMessage(message->chat->id, "Your data has been loaded from the file.", false, 0, createBottomKeyboard());
            }
            else {
                bot.getApi().sendMessage(message->chat->id, "Your data is incomplete.", false, 0, createBottomKeyboard());
            }
        }
        else {
            if (!isRegistered) {
                bot.getApi().sendMessage(message->chat->id, "Do you want to register? (Yes/No)", false, 0, createBottomKeyboard());
            }
            else {
                bot.getApi().sendMessage(message->chat->id, "You are already registered.", false, 0, createBottomKeyboard());
            }
        }
        });


    bot.getEvents().onAnyMessage([&bot, &userMap, &isRegistered, &isWaitingForName, &isWaitingForAge, &isWaitingForPass](Message::Ptr message) {
        printf("Message from: %s\n", message->from->username.c_str());
        printf("Text: %s\n", message->text.c_str());
        int userId = message->from->id;

        if (!isRegistered) {
            if (isWaitingForName) {
                userMap[userId].SetName(message->text);
                isWaitingForName = false;
                bot.getApi().sendMessage(message->chat->id, "Type your age: ", false, 0, createBottomKeyboard());
                isWaitingForAge = true;
            }
            else if (isWaitingForAge) {
                try {
                    int age = stoi(message->text);
                    userMap[userId].SetAge(age);
                    isWaitingForAge = false;
                    bot.getApi().sendMessage(message->chat->id, "Type your password (only numbers): ", false, 0, createBottomKeyboard());
                    isWaitingForPass = true;
                }
                catch (const invalid_argument& e) {
                    bot.getApi().sendMessage(message->chat->id, "Please type a valid age.", false, 0, createBottomKeyboard());
                }
            }
            else if (isWaitingForPass) {
                try {
                    int password = stoi(message->text);
                    userMap[userId].SetPassword(password);
                    isWaitingForPass = false;
                    isRegistered = true;
                    bot.getApi().sendMessage(message->chat->id, "Registration completed.", false, 0, createBottomKeyboard());
                }
                catch (const invalid_argument& e) {
                    bot.getApi().sendMessage(message->chat->id, "Please type a valid password.", false, 0, createBottomKeyboard());
                }
            }
            else if (message->text == "Yes" || message->text == "yes") {
                bot.getApi().sendMessage(message->chat->id, "Type your name (only english): ", false, 0, createBottomKeyboard());
                isWaitingForName = true;
            }
        }
        });

    bot.getEvents().onCommand("info", [&bot, &userMap](Message::Ptr message) {
        int userId = message->from->id;
        string name = userMap[userId].GetName();

        if (!name.empty()) {
            bot.getApi().sendMessage(message->chat->id, "Your name: " + name, false, 0, createBottomKeyboard());
        }
        else {
            bot.getApi().sendMessage(message->chat->id, "Your name is not provided yet.", false, 0, createBottomKeyboard());
        }

        bot.getApi().sendMessage(message->chat->id, "Your age: " + to_string(userMap[userId].GetAge()), false, 0, createBottomKeyboard());
        bot.getApi().sendMessage(message->chat->id, "Do you want to see your password?", false, 0, createInlineYesNoKeyboard());
        });

    bot.getEvents().onCommand("writefile", [&bot, &userMap](Message::Ptr message) {
        int userId = message->from->id;
        string filename = "user_" + to_string(userId) + "_" + message->from->username.c_str() + "_data.txt";
        ifstream inFile(filename);
        bool usernameExists = false;

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

                bot.getApi().sendMessage(message->chat->id, "Your data is written.", false, 0, createBottomKeyboard());
                outFile.close();
            }
        }
        else {
            bot.getApi().sendMessage(message->chat->id, "Your username is already in the file.", false, 0, createBottomKeyboard());
        }
        });

    bot.getEvents().onCommand("links", [&bot](Message::Ptr message) {
        InlineKeyboardMarkup::Ptr keyboard(new InlineKeyboardMarkup);
        InlineKeyboardButton::Ptr button(new InlineKeyboardButton);

        button->text = "Github";
        button->url = "https://github.com/illusiOxd";
        keyboard->inlineKeyboard.push_back({ button });

        bot.getApi().sendMessage(message->chat->id, "Click that buttons to follow the link.", false, 0, keyboard);
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
