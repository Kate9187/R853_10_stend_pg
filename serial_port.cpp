/* Работа с COM-портом, формирование пакетов для отправки */

#include "serial_port.h"

SerialPort::SerialPort() :
    com(new QSerialPort)
{
}

SerialPort::~SerialPort()
{
   if(com->isOpen()) com->close();
   delete com;
}

//Открытие порта
//args: comName - имя порта
//return: true/false - порт открыт/не открыт
bool SerialPort::open(QString comName){

    com->setPortName(comName);
    if (com->open(QIODevice::ReadWrite)) // порт открыт
    {
        //Настройка COM-порта
        com->setBaudRate(115200, QSerialPort::AllDirections);
        com->setDataBits(QSerialPort::Data8);
        com->setParity(QSerialPort::NoParity);
        com->setStopBits(QSerialPort::OneStop);

        return true;
    }
    else return false;

}

//Закрытие порта
//return: true/false - порт закрыт/не закрыт
bool SerialPort::close(){
    com->close();
    if(!com->isOpen()) return true;
    else{
        emit comErrorSignal(com->errorString()); //сигнал с текстом сообщения об ошибке при работе с COM-портом
        return false;
    }
}

//Вычисление контрольной последовательности сообщения табличным способом
//args: msg - сообщение
//return: результат вычисления (остаток от деления полиномов)
quint16 SerialPort::CalcCRC(QByteArray &msg){

    int n = msg.length();
    quint8* buff = new quint8[n];
    memcpy(buff, msg.data(), n);

    quint8 CRCLo = 0xFF;   // младший байт регистра CRC
    quint8 CRCHi = 0xFF;   // старший байт регистра CRC
    uint  Index;          // индекс для таблиц

    for(int i = 0; i < n; i++){

        Index = CRCLo ^ buff[i];
        CRCLo = CRCHi ^ CRCLo_Table[Index];
        CRCHi = CRCHi_Table[Index];
    }

    return ((quint16)CRCHi << 8 | CRCLo);
}

//Конфигурирование адаптера
void SerialPort::set_config(){
    //Перепад сигнала DTR из 1 в 0
    com->setDataTerminalReady(true);
    com->setDataTerminalReady(false);

    //Установка команды 1 (работа с платой КФ1)
    QByteArray tmp;
    tmp.append(1);
    if(com->write(tmp) != -1) hexStrSpace(0, tmp.length(), tmp); //отправленное сообщение выводится в сниффер
    else{
        emit errorSignal(" Команда не отправлена");
        return;
    }

    //Проверка ответа адаптера (бит 7)
    if(com->waitForReadyRead(100)){ //ожидание входных данных в течение 100 мс
        tmp.clear();
        tmp = com->readAll();
        if(tmp.isEmpty()){
            emit errorSignal("Ошибка чтения данных: " + com->errorString());
            return;
        }
        hexStrSpace(1, tmp.length(), tmp); //полученное сообщение выводится в сниффер
        quint8 byte = tmp[0];
        if((byte >> 7) == 1){
            QEventLoop loop;
            QTimer::singleShot(100, &loop, SLOT(quit())); //блокировка на 100 мс после установки режима
            loop.exec();
            //Устанавливаются режим и поддиапазон (ф-я 10), ЦАП (ф-я 6)
            forConfig = true;
            func_10H_F5361(true);
            emit F5_SelectSignal(3);
            func_6H(1);
            emit valueCAPSignal(1);
            emit F6_SelectSignal(1);
            emit successSignal(" Успешное конфигурирование адаптера");
        }
        else emit (errorSignal(" Неверный ответ адаптера"));
    }
    else emit (errorSignal(" Адаптер не отвечает на конфигурирование"));
}

//Формирование и отправка пакета (ф-ия 10)
//args: address_byte - адресный байт, dataByte - байт данных
void SerialPort::func_10H(quint8 address_byte, quint8 dataByte){
    QByteArray arr_10H, answ;
    quint16 x;

    arr_10H.append(0x01);
    arr_10H.append(0x10);
    arr_10H.append((char)0x00);
    arr_10H.append(0x1E);
    arr_10H.append((char)0x00);
    arr_10H.append(0x03);
    arr_10H.append(0x06);
    arr_10H.append((char)0x00);
    arr_10H.append((char)0x00);
    arr_10H.append(address_byte);
    arr_10H.append(dataByte);
    arr_10H.append((char)0x00);
    arr_10H.append(0x10);
    x = CalcCRC(arr_10H);
    arr_10H.append(x & 0xFF);
    arr_10H.append((x >> 8) & 0xFF);

    if(com->write(arr_10H)!=-1){
        emit successSignal(" Отправлено");
        emit borderSignal();
        hexStrSpace(0, arr_10H.toHex().length(), arr_10H);
    }
    else emit errorSignal(" Настройки не отправлены");

    if(com->waitForReadyRead(50)){ //ожидание ответа адаптера в течение 50 мс
        answ = com->readAll();
        hexStrSpace(1, arr_10H.toHex().length(), arr_10H);
    }
    else emit noAnswSignal();
}

//Формирование битов данных для 6 модуля 8 регистра 10 ф-ии
//args: ind - индекс выбранного элемента списка, table_F - список
void SerialPort::func_10H_68(int ind, int table_F = 0)
{
    if(table_F==5){
        dataByte68 = dataByte68 >> 4;
        dataByte68 = dataByte68 << 4;

        switch (ind) {
        case 0:
            break;
        case 1:
            dataByte68 |= 0b1000;
            break;
        case 2:
            dataByte68 |= 0b0100;
            break;
        case 3:
            dataByte68 |= 0b0010;
            break;
        default:
            emit errorSignal(" Невозможно установить выбранные настройки");
            return;
        }
        indElem={listV_F5}; //флаг на активированный элемент формы
        func_10H(0x68, dataByte68); //отправка сообщения
    }
    else if(table_F==6){
        quint8 b = dataByte68 & 0b1111;
        quint8 a = 0;
        switch (ind) {
        case 0:
            a |= 0b0100;
            break;
        case 1:
            a |= 0b0010;
            break;
        case 2:
            a |= 0b0001;
            break;
        default:
            emit errorSignal(" Невозможно установить выбранные настройки");
            return;
        }
        dataByte68 = 0;
        dataByte68 |= a;
        dataByte68 = dataByte68  << 4;
        dataByte68 |= b;

        indElem={listV_F6}; //флаг на активированный элемент формы
        func_10H(0x68, dataByte68); //отправка сообщения
    }
}

//Формирование битов данных для 6 модуля 2 регистра 10 ф-ии
//args: n - выбранная настройка
void SerialPort::func_10H_62(int n)
{
    switch (n) {
    // Вкл/выкл
    case 0:
        dataByte62 = dataByte62^0b10000000;
        indElem={rButt_F0}; //флаг на активированный элемент формы
        break;
    // Доп режимы
    case 1:
        dataByte62 = dataByte62 >> 3;
        dataByte62 = dataByte62^0b1000;
        dataByte62 = dataByte62 << 3;
        indElem={listW_CheckdB};
        break;
    case 2:
        dataByte62 = dataByte62 >> 3;
        dataByte62 = dataByte62^0b0100;
        dataByte62 = dataByte62 << 3;
        indElem={listW_CheckdB};
        break;
    case 3:
        dataByte62 = dataByte62 >> 3;
        dataByte62 = dataByte62^0b0010;
        dataByte62 = dataByte62 << 3;
        indElem={listW_CheckdB};
        break;
    case 4:
        dataByte62 = dataByte62 >> 3;
        dataByte62 = dataByte62^0b0001;
        dataByte62 = dataByte62 << 3;
        indElem={listW_CheckdB};
        break;
    case 14:
        dataByte62 = dataByte62 >> 7;
        dataByte62 = dataByte62 << 7;
        indElem={pB_reset};
        break;
    default:
        emit errorSignal(" Невозможно установить выбранные настройки");
        return;
    }
    func_10H(0x62, dataByte62); //отправка сообщения
}

//Стилизация сообщения для сниффера
//args: n - сообщение получено/отправлено (true/false), size - размер сообщения, arr - сообщение
void SerialPort::hexStrSpace(bool in, int size, QByteArray &arr){

    QStringList strList;
    QString str = ""; 
    for(int i = 0; i < size; i+=2)
    {
        strList.append(arr.toHex().mid(i, 2));
    }

    if(in)
    {
        str.append("-> ");
    }
    else
    {
        str.append("<- ");
    }

    for(int i = 0; i < strList.size(); i++)
    {
        str.append(strList.at(i));
        str.append(" ");
    }

    emit snifferSignal(str); //сигнал с текстом сообщения для сниффера
}

//Формирование и отправка пакета (ф-ия 6)
//args: value - значение ЦАП от 0 до 1023
void SerialPort::func_6H(quint16 value)
{
    QByteArray arr_6H, answ;
    quint16 CRC;

    arr_6H.append(0x01);
    arr_6H.append(0x06);
    arr_6H.append((char) 0x00);
    arr_6H.append(0x0A);
    if(value < 255){
        arr_6H.append((char) 0x00);
        arr_6H.append(value);
    }
    else{

        arr_6H.append(value >> 8);
        arr_6H.append(value & 0xFF);
    }
    CRC = CalcCRC(arr_6H);
    arr_6H.append(CRC & 0xFF);
    arr_6H.append((CRC >> 8) & 0xFF);

    if(forConfig) forConfig = false;
    else indElem={sBox_valueCAP}; //флаг на активированный элемент формы

    //Отправка пакета
    if(com->write(arr_6H)!=-1){
        emit successSignal(" Отправлено");
        emit borderSignal();
        hexStrSpace(0, arr_6H.toHex().length(), arr_6H); //отправленное сообщение выводится в сниффер
    }
    else emit errorSignal(" Настройки не отправлены");

    //Ожидание ответа адаптера в течение 50 мс
    if(com->waitForReadyRead(50)){
        answ = com->readAll();
        hexStrSpace(1, arr_6H.toHex().length(), arr_6H); //полученное сообщение выводится в сниффер
    }
    else emit noAnswSignal();

}

//Вкл/выкл генератора шума
//args: checked - режим генератора шума вкл/выкл (true/false)
void SerialPort::func_10H_F5361(bool checked)
{
    if(!forConfig) indElem={pB_5361};

    //Вкл устанавливает 3 эл-т списка режимов, 1 поддиапазонов
    if(checked){
        dataByte68 = 0x22;
        func_10H(0x68, dataByte68);
        emit F5_SelectSignal(3);
        emit F6_SelectSignal(1);
    }
    //Выкл 0 эл-т списка режимов, 1 поддиапазонов
    else{
        dataByte68 = 0x20;
        func_10H(0x68, dataByte68);
        emit F5_SelectSignal(0);
        emit F6_SelectSignal(1);
    }
}
