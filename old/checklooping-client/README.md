# Проверка модулей на зацикливание.  

## 1. Требования к ОС:  
ubuntu


## 2. Краткое описание:

В диспетчере реализован функционал, позволяющий контролировать каждый модуль на зацикливание и передавать от модуля в диспетчер статус состояния.
Для связи с диспетчером используется shared memory.
Если в контролируемом цикле перестанет передаваться статус, то диспетчер обнаружит это и через некоторое время перезапустит модуль.


## 3. Интеграция в проект

Для использования контроля зацикливания в модуле нужно добавить в проект три файла:
- **u-checkloop-data.hpp**
- **u-checkloop-client.hpp**
- **u-checkloop-client.cpp**
Понадобится также подключить стандартную библиотеку **librt** ( **-lrt** ) для работы с shared memory


## 4. Пример:

Пример использования функций для контроля зависаний модуля:
```cpp
#include <u-checkloop-client.hpp>
#include <unistd.h>
#include <stdio.h>


bool enaWork = true;


int main()
{
    gtpCheckLoopClient dispatcherStatus;

    if (!dispatcherStatus.Open()) {
        fprintf(stderr, "Не удалось подключиться к диспетчеру");
    }

    while (enaWork) {
        dispatcherStatus.Notify();
        sleep(1);
    }
    dispatcherStatus.Close();
}
```
