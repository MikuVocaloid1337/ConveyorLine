#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include <random>

using namespace std;

mutex conveyorMutex;
vector<int> defectiveBodies; // Список бракованных кузовов
vector<bool> blockedSlots(10, false); // Заблокированные ячейки

bool isDefective() {
    static random_device rd;
    static mt19937 gen(rd());
    static uniform_int_distribution<int> dis(1, 10);
    return dis(gen) == 1; // 10% шанс брака
}

void produceBody(int bodyNumber, vector<pair<bool, bool>>& bodies, int index) {
    bool defective = isDefective();
    {
        lock_guard<mutex> lock(conveyorMutex);
        bodies[index] = { true, defective };
        cout << "Производство кузова " << bodyNumber << (defective ? " (БРАК)" : "") << " началось." << endl;
        if (defective) {
            defectiveBodies.push_back(bodyNumber);
            blockedSlots[index] = true; // Блокировка ячейки
        }
    }

    for (int step = 0; step < 3; ++step) {
        {
            lock_guard<mutex> lock(conveyorMutex);
            for (size_t i = 0; i < bodies.size(); ++i) {
                if (blockedSlots[i]) {
                    cout << "[!] ";
                }
                else {
                    cout << (bodies[i].first ? "[X]" : "[ ]") << " ";
                }
            }
            cout << endl;
        }
        this_thread::sleep_for(chrono::milliseconds(500));
    }

    {
        lock_guard<mutex> lock(conveyorMutex);
        if (defective) {
            cout << "Рабочие удаляют бракованный кузов " << bodyNumber << " с линии, но ячейка остаётся заблокированной." << endl;
        }
        else {
            bodies[index] = { false, false };
            cout << "Кузов " << bodyNumber << " завершён и ушёл с линии." << endl;
        }
    }
}

int main() {
    setlocale(LC_ALL, "ru");
    const int SIZE = 10;
    vector<pair<bool, bool>> bodies(SIZE, { false, false }); // {производится, бракованный}
    int bodyNumber = 1; // Счетчик номеров кузовов

    vector<thread> threads;

    for (int i = 0; i < 20; ++i) { // Симуляция 20 кузовов
        int index = (bodyNumber - 1) % SIZE; // Цикличный индекс
        if (blockedSlots[index]) {
            cout << "Кузов " << bodyNumber << " не может быть произведён, так как ячейка " << index + 1 << " заблокирована." << endl;
            bodyNumber = (bodyNumber % SIZE) + 1;
            this_thread::sleep_for(chrono::milliseconds(300));
            continue;
        }
        threads.emplace_back(produceBody, bodyNumber, ref(bodies), index);
        bodyNumber = (bodyNumber % SIZE) + 1; // Следующий номер
        this_thread::sleep_for(chrono::milliseconds(300)); // Имитация поступления нового кузова
    }

    for (auto& t : threads) {
        t.join();
    }

    cout << "Бракованные кузова: ";
    for (int num : defectiveBodies) {
        cout << num << " ";
    }
    cout << endl;

    system("pause");
    return 0;
}