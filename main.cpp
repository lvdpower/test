#include <atomic>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <openssl/md5.h>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

// флаг чтоб потоки могли знать найдено значение каким либо другим потоком или еще нет
std::atomic<bool> gotvalue(false);

// функция возвращает MD5 hash от введенной строки
std::string cal_md5(std::string_view text)
{
    unsigned char hash[MD5_DIGEST_LENGTH];
    char hashHex[33];
    MD5(reinterpret_cast<const unsigned char *>(text.data()), text.length(), hash);
    sprintf_s(hashHex, "%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x", hash[0], hash[1], hash[2],
              hash[3], hash[4], hash[5], hash[6], hash[7], hash[8], hash[9], hash[10], hash[11], hash[12], hash[13],
              hash[14], hash[15]);

    return hashHex;
}

// функция принимает диапазон пин-кодов, в котором ищет тот, который подходит к заданному хэшу
void find_pincode(size_t startval, size_t endval, std::string_view hashcode)

{

    for (size_t i = startval; i <= endval; ++i)

    { // если какой то другой поток уже нашел пин-код, то выходим из функции
        if (gotvalue.load())
        {
            break;
            return;
        }
        std::string num_str = std::to_string(i);
        num_str.insert(0, 8 - num_str.size(), '0'); // чтобы впереди были значащие нули, то есть
                                                    // допустим число 1 преобразовалось в строку 00000001

        if (hashcode == cal_md5(num_str)) // тут проверяем подходит ли заданный хэш к хэшу пин-кода

        {
            std::cout << std::endl
                      << "The pin code for provided md5hash is: " << std::setfill('0') << std::setw(8) << i
                      << std::endl;
            gotvalue = true; // ставим отметку, что пин-код найден, чтоб другие потоки могли зря не работать
            break;           // когда пин-код найден можно прервать цикл
        }
    };
}

int main(int argc, char **argv)
{
    auto num_threads = std::thread::hardware_concurrency(); //узнаем число возможных потоков

    if (argc != 2)
    { // говорим, что нужен аргумент - md5 хэш для работы программы
        std::cout << "You must input a string md5 hash value as the argument";
        return -1;
    }
    else if (num_threads < 2) // по условию задачи допускатеся работа только в многопоточном режиме
    {
        std::cout << "CPU must have multi kernels at least two" << std::endl;
        return -1;
    }

    auto hashvalue = argv[1]; //аргумент командной строки это хэш md5, от которой будем искать её пин-код
    constexpr size_t maxpinval = 99999999; // это максимальное значение пин-кода из 8 цифр.

    //находим кол-во итераций для всех потоков, кроме последнего
    size_t thread_iter = (maxpinval + 1) / num_threads;

    //остаточное кол-во итераций на последний поток
    size_t thread_last_iter = (maxpinval + 1) - (thread_iter * (num_threads - 1));

    std::vector<std::thread> workthreads;

    for (int n = 0; n < (int)(num_threads - 1); ++n)
    {
        workthreads.emplace_back(find_pincode, (n * thread_iter) + n, (n + 1) * thread_iter, hashvalue);
    }

    // последний поток выполняет перебор остатка диапазона
    workthreads.emplace_back(find_pincode, maxpinval - thread_last_iter, maxpinval, hashvalue);

    std::cout << std::endl << "Running number of threads: " << workthreads.size() << std::endl;

    // запускаем и присоедняем потоки
    for (std::thread &t : workthreads)
    {
        t.join();
    }

    return 0;
}
