#include <stdio.h>
#include <tgbot/tgbot.h>
#include <vector>

using namespace TgBot;
using namespace std;

class TgUser {
private:
    string name;
    int age;

public:
    TgUser()
    {
        name = " ";
        age = -1;
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
};

int main()
{
    Bot bot("6927127350:AAGWg0O3VpP3e0o0HNySeW81SgRbUYwmoPY");
    TgUser PersonalUser;
    bool isRegistered = false;
    bool isWaitingForName = false, isWaitingForAge = false; 


    bot.getEvents().onAnyMessage([&bot, &PersonalUser, &isRegistered, &isWaitingForName, &isWaitingForAge](Message::Ptr message) {
        if (!isRegistered)
        {
            if (isWaitingForName)
            {
                // Пользователь уже ввел имя
                PersonalUser.SetName(message->text); // Записываем имя пользователя
                isWaitingForName = false; // Пользователь ввел имя, больше не ждем
                bot.getApi().sendMessage(message->chat->id, "Type your age: ");
                isWaitingForAge = true; // Ожидаем ввод возраста
            }
            else if (isWaitingForAge)
            {
                // Пользователь уже ввел возраст
                try {
                    int age = stoi(message->text);
                    PersonalUser.SetAge(age);
                    isWaitingForAge = false; // Пользователь ввел возраст, больше не ждем
                    isRegistered = true; // Регистрация завершена
                    bot.getApi().sendMessage(message->chat->id, "Registration completed.");
                }
                catch (const invalid_argument& e) {
                    bot.getApi().sendMessage(message->chat->id, "Please type a valid age.");
                }
            }
            else if (message->text == "Y")
            {
                // Пользователь согласился зарегистрироваться
                bot.getApi().sendMessage(message->chat->id, "Type your name: ");
                isWaitingForName = true; // Ожидаем ввод имени
            }
            else
            {
                // Пользователь отказался от регистрации
                bot.getApi().sendMessage(message->chat->id, "Do you want to register? Y/N");
            }
        }
        });

    bot.getEvents().onCommand("info", [&bot, &PersonalUser](Message::Ptr message) {
        string name = PersonalUser.GetName();
        if (!name.empty()) {
            bot.getApi().sendMessage(message->chat->id, "Your name: " + name);
        }
        else {
            bot.getApi().sendMessage(message->chat->id, "Your name is not provided yet.");
        }
        bot.getApi().sendMessage(message->chat->id, "Your age: " + to_string(PersonalUser.GetAge()));
        });

    bot.getEvents().onAnyMessage([&bot](Message::Ptr message) {
        printf("User wrote %s\n", message->text.c_str());
        if (StringTools::startsWith(message->text, "/start")) {
            return;
        }
        });

    /*bot.getEvents().onCommand("sendpoll", [&bot](Message::Ptr message) {
    Poll::Ptr poll(new Poll);
    poll->question = "Какой ваш любимый цвет?";

    poll->options.push_back(std::make_shared<PollOption>("Синий"));
    poll->options.push_back(std::make_shared<PollOption>("Зеленый"));
    poll->options.push_back(std::make_shared<PollOption>("Красный"));

    poll->isAnonymous = false;
    poll->allowsMultipleAnswers = false;
    poll->openPeriod = 60;
    poll->type = "Regular";

    std::vector<std::string> pollOptionTexts;
    for (const auto& option : poll->options) {
        pollOptionTexts.push_back(option->text);
    }

    bot.getApi().sendPoll(message->chat->id, poll->question, pollOptionTexts, poll->isAnonymous,
        poll->allowsMultipleAnswers, 0, "", 0, false, poll->openPeriod);
    });*/
    try
    {
        printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
        TgLongPoll longPoll(bot);
        while (true)
        {
            printf("Long poll started\n");
            longPoll.start();
        }
    }
    catch (TgException& e)
    {
        printf("error: %s\n", e.what());
    }
    return 0;
}
