#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "serial_port.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    serialPort(new SerialPort)
{
    ui->setupUi(this);

    //Фиксированный размер формы
    setMinimumSize(QSize(560,340));
    setMaximumSize(QSize(560,340));

    //Конфигурирование адаптера недоступно до открытия порта
    ui->pB_config->setEnabled(false);

    //Список доступных портов
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        ui->cB_ComList->addItem(info.portName() + " " + info.description(), info.portName());
    }

    if(ui->cB_ComList->currentIndex()==-1){
        ui->cB_ComList->addItem("COM-порт не найден");
        ui->cB_ComList->setEnabled(false);
    }

    connect(serialPort, SIGNAL(comErrorSignal(QString)), this, SLOT(errorSlot(QString)));
    connect(serialPort, SIGNAL(successSignal(QString)), this, SLOT(successSlot(QString)));
    connect(this, SIGNAL(errorSignal(QString)), this, SLOT(errorSlot(QString)));
    connect(serialPort, SIGNAL(errorSignal(QString)), this, SLOT(errorSlot(QString)));
    connect(serialPort, SIGNAL(snifferSignal(QString)), this, SLOT(snifDataSlot(QString)));
    connect(serialPort, SIGNAL(F5_SelectSignal(int)), this, SLOT(F5_SelectSlot(int)));
    connect(serialPort, SIGNAL(F6_SelectSignal(int)), this, SLOT(F6_SelectSlot(int)));
    connect(serialPort, SIGNAL(noAnswSignal()), this, SLOT(noAnswSlot()));
    connect(serialPort, SIGNAL(borderSignal()), this, SLOT(borderColorSlot()));
    connect(serialPort, SIGNAL(valueCAPSignal(int)), this, SLOT(valueCAPSlot(int)));

    //Список режимов
    QStringList listF5 = (QStringList() << "Выключено" << "Приём" << "Калибровка" << "Генератор шума");
    QStringListModel* modelF5 = new QStringListModel(listF5);
    ui->listV_F5->setModel(modelF5);
    ui->listV_F5->setEditTriggers(QAbstractItemView::NoEditTriggers);

    //Список поддиапазонов
    QStringList listF6 = (QStringList() << "30-70 МГц" << "70-100 МГц" << "100-400 МГц");
    QStringListModel* modelF6 = new QStringListModel(listF6);
    ui->listV_F6->setModel(modelF6);
    ui->listV_F6->setEditTriggers(QAbstractItemView::NoEditTriggers);

    //Элементы формы недоступны до открытия порта
    ui->checkB_F1->setEnabled(false);
    ui->checkB_F2->setEnabled(false);
    ui->checkB_F3->setEnabled(false);
    ui->checkB_F4->setEnabled(false);
    ui->listV_F5->setEnabled(false);
    ui->listV_F6->setEnabled(false);
    ui->rButt_F0->setEnabled(false);
    ui->sBox_valueCAP->setEnabled(false);
    ui->pB_EnterCAP->setEnabled(false);
    ui->pB_reset->setEnabled(false);
    ui->pB_sniffClear->setEnabled(false);
    ui->pB_5361->setEnabled(false);

    ui->tE_sniffer->setReadOnly(true);
    ui->tE_sniffer->setStyleSheet("QTextEdit QScrollBar {border: 2px solid grey;}");

    ui->sBox_valueCAP->setRange(0,1023); //значения ЦАП от 0 до 1023
}

MainWindow::~MainWindow()
{
    delete ui;
    delete serialPort;
}


//Установка значения ЦАП визуально на форме
//args: val - значение ЦАП
void MainWindow::valueCAPSlot(int val)
{
    ui->sBox_valueCAP->setValue(1);
}

//Обработка нажатия горячих клавиш
//args: key - нажатая клавиша
void MainWindow::keyReleaseEvent(QKeyEvent *key)
{
    int k1 = key->key();
    if (k1 == Qt::Key_F5)
    {
        //Переход на следующий элемент списка режимов (F5)
        if(ui->listV_F5->isEnabled()){
            int currInd = ui->listV_F5->currentIndex().row();
            if(currInd==3)currInd=0; //в списке 4 элемента
            else currInd++;
            QModelIndex newInd = ui->listV_F5->model()->index(currInd,0);
            ui->listV_F5->clearSelection();
            ui->listV_F5->clicked(newInd); //генерация сигнала нажатия элемента списка
            ui->listV_F5->setCurrentIndex(newInd); //визуальное выделение элемента списка
        }
    }
    if (k1 == Qt::Key_F6)
    {
        //Переход на следующий элемент списка поддиапазонов (F6)
        if(ui->listV_F6->isEnabled()){
            int currInd = ui->listV_F6->currentIndex().row();
            if(currInd==2)currInd=0; //в списке 3 элемента
            else currInd++;
            QModelIndex newInd = ui->listV_F6->model()->index(currInd,0);
            ui->listV_F6->clearSelection();
            ui->listV_F6->clicked(newInd); //генерация сигнала нажатия элемента списка
            ui->listV_F6->setCurrentIndex(newInd); //визуальное выделение элемента списка
        }
    }
    if (k1 == Qt::Key_F7)
    {
        //Генерация сигнала нажатия кнопки генератора шума (F7)
        if(ui->pB_5361->isEnabled()){

            on_pB_5361_clicked();
        }
    }
    if (k1 == Qt::Key_F8)
    {
        //Установка фокуса для ввода ЦАП (F8)
        if(ui->sBox_valueCAP->isEnabled()){

            ui->sBox_valueCAP->setFocus();
        }
    }
    if (k1 == Qt::Key_F9)
    {
        //Вкл/выкл аттенюатора (F9)
        if(ui->rButt_F0->isChecked()){
            ui->rButt_F0->setChecked(false); //визуальное представление (вкл/выкл)
            on_rButt_F0_clicked(false); //генерация сигнала нажатия кнопки
        }
        else{
            ui->rButt_F0->setChecked(true);
            on_rButt_F0_clicked(true);
        }
    }
    // Усновка настроек аттенюатора (F1, F2, F3, F4 )
    if (k1 == Qt::Key_F1)
    {
        if(ui->checkB_F1->isEnabled()){
            serialPort->func_10H_62(1); //инициируем процесс формирования и отправки пакета
            if(ui->checkB_F1->isChecked())ui->checkB_F1->setChecked(false); //визуальное представление (вкл/выкл)
            else ui->checkB_F1->setChecked(true);
        }
    }
    if (k1 == Qt::Key_F2)
    {
        if(ui->checkB_F2->isEnabled()){
            serialPort->func_10H_62(2);
            if(ui->checkB_F2->isChecked())ui->checkB_F2->setChecked(false);
            else ui->checkB_F2->setChecked(true);
        }
    }
    if (k1 == Qt::Key_F3)
    {
        if(ui->checkB_F3->isEnabled()){
            serialPort->func_10H_62(3);
            if(ui->checkB_F3->isChecked())ui->checkB_F3->setChecked(false);
            else ui->checkB_F3->setChecked(true);
        }
    }
    if (k1 == Qt::Key_F4)
    {
        if(ui->checkB_F4->isEnabled()){
            serialPort->func_10H_62(4);
            if(ui->checkB_F4->isChecked())ui->checkB_F4->setChecked(false);
            else ui->checkB_F4->setChecked(true);
        }
    }
}

//Визуальный эффект отправки данных на форме
void MainWindow::borderColorSlot(){

    //Блокировка всех элементов на форме
    ui->listV_F5->setEnabled(false);
    ui->listV_F6->setEnabled(false);
    ui->pB_5361->setEnabled(false);
    ui->pB_EnterCAP->setEnabled(false);
    ui->rButt_F0->setEnabled(false);
    if(ui->rButt_F0->isChecked()){
        ui->checkB_F1->setEnabled(false);
        ui->checkB_F2->setEnabled(false);
        ui->checkB_F3->setEnabled(false);
        ui->checkB_F4->setEnabled(false);
    }
    ui->sBox_valueCAP->setEnabled(false);

    //Изменение внешнего вида выбранного элемента
    switch (serialPort->indElem) {
    case 1:
        ui->listV_F5->setStyleSheet("QListView {border: 2px solid #55aa7f;}"); //задание стиля отправки данных
        QTimer::singleShot(300, this, SLOT(borderInactSlot())); //пауза 300 мс и возвращение к стандартому стилю
        break;
    case 2:
        ui->listV_F6->setStyleSheet("QListView {border: 2px solid #55aa7f;}");
        QTimer::singleShot(300, this, SLOT(borderInactSlot()));
        break;
    case 3:
        ui->listW_CheckdB->setStyleSheet("QListView {border: 2px solid #55aa7f;}");
        QTimer::singleShot(300, this, SLOT(borderInactSlot()));
        break;
    case 4:
        ui->listW_CheckdB->setStyleSheet("QListView {border: 2px solid #55aa7f;}");
        QTimer::singleShot(300, this, SLOT(borderInactSlot()));
        break;
    case 5:
        ui->sBox_valueCAP->setStyleSheet("QSpinBox {background-color: #55aa7f;}");
        QTimer::singleShot(300, this, SLOT(borderInactSlot()));
        break;
    case 6:
        ui->pB_5361->setStyleSheet("QPushButton {background-color: #55aa7f;}");
        QTimer::singleShot(300, this, SLOT(borderInactSlot()));
        break;
    case 0:
        QTimer::singleShot(300, this, SLOT(borderInactSlot()));
        break;
    default:
        break;
    }
}

//Сброс визуального эффекта отправки данных
void MainWindow::borderInactSlot(){

    //Возвращение элементов в активное состояние
    ui->listV_F5->setEnabled(true);
    ui->listV_F6->setEnabled(true);
    ui->pB_5361->setEnabled(true);
    ui->pB_EnterCAP->setEnabled(true);
    ui->rButt_F0->setEnabled(true);
    if(ui->rButt_F0->isChecked()){
        ui->checkB_F1->setEnabled(true);
        ui->checkB_F2->setEnabled(true);
        ui->checkB_F3->setEnabled(true);
        ui->checkB_F4->setEnabled(true);
    }
    ui->sBox_valueCAP->setEnabled(true);

    //Установка стандарного стиля выбранного элемента
    switch (serialPort->indElem) {
    case 1:
        ui->listV_F5->setStyleSheet("");
        break;
    case 2:
        ui->listV_F6->setStyleSheet("");
        break;
    case 3:
        ui->listW_CheckdB->setStyleSheet("");
        break;
    case 4:
        ui->listW_CheckdB->setStyleSheet("");
        break;
    case 5:
        ui->sBox_valueCAP->setStyleSheet("");
        break;
    case 6:
        ui->pB_5361->setStyleSheet("");
        break;
    default:
        break;
    }
}

//Визуальное выделение элемента в списке режимов
void MainWindow::F5_SelectSlot(int ind){
    ui->listV_F5->clearSelection();
    QModelIndex indMod = ui->listV_F5->model()->index(ind,0);
    ui->listV_F5->selectionModel()->select(indMod, QItemSelectionModel::Select);
}

//Визуальное выделение элемента в списке поддиапазонов
void MainWindow::F6_SelectSlot(int ind){
    ui->listV_F6->clearSelection();
    QModelIndex indMod = ui->listV_F6->model()->index(ind,0);
    ui->listV_F6->selectionModel()->select(indMod, QItemSelectionModel::Select);
}


//Вывод сообщения об ошибке на форму
//args: error - текст сообщения
void MainWindow::errorSlot(QString error = ""){
    QString errorName;
    if(!error.isEmpty()) errorName = " " + error;
    else errorName = " Неопознанная ошибка";
    ui->label_info->setStyleSheet("QLabel {background-color: red;}");
    ui->label_info->setText(errorName);
    QTimer::singleShot(3000, this, SLOT(inactiveSlot())); //сообщение будет показано в течение 3000 мс
}

//Вывод сообщения об отсутствии ответа на форму
void MainWindow::noAnswSlot(){
    ui->label_info->setStyleSheet("QLabel {background-color: yellow;}");
    ui->label_info->setText(" Нет ответа");
    QTimer::singleShot(300, this, SLOT(inactiveSlot())); //сообщение будет показано в течение 300 мс
}

//Вывод сообщения об успешной операции на форму
//args: msg - текст сообщения
void MainWindow::successSlot(QString msg){
    ui->label_info->setStyleSheet("QLabel {background-color: #55aa7f;}");
    ui->label_info->setText(" " + msg);
    QTimer::singleShot(300, this, SLOT(inactiveSlot())); //сообщение будет показано в течение 300 мс
}

//Сброс информационного сообщения на форме
void MainWindow::inactiveSlot(){
    ui->label_info->setStyleSheet("");
    ui->label_info->setText("");
}

//Открытие/закрытие порта
void MainWindow::on_pB_ComOpen_clicked()
{
    if(ui->pB_ComOpen->text()=="Открыть"){
        QString comName = ui->cB_ComList->currentData().toString(); //получение имени выбранного порта
        if(serialPort->open(comName)){
            //Если порт открыт, разблокировка элементов на форме
            ui->pB_ComOpen->setText("Закрыть");
            ui->cB_ComList->setEnabled(false);
            ui->pB_config->setEnabled(true);
            ui->listV_F5->setEnabled(true);
            ui->listV_F6->setEnabled(true);
            ui->rButt_F0->setEnabled(true);
            ui->sBox_valueCAP->setEnabled(true);
            ui->pB_EnterCAP->setEnabled(true);
            ui->pB_sniffClear->setEnabled(true);
            ui->pB_reset->setEnabled(true);
            ui->pB_5361->setEnabled(true);
            if(ui->rButt_F0->isChecked()){
                ui->checkB_F1->setEnabled(true);
                ui->checkB_F2->setEnabled(true);
                ui->checkB_F3->setEnabled(true);
                ui->checkB_F4->setEnabled(true);
            }
        }
        else emit errorSignal(" Не удалось открыть порт");
    }
    else{
        if(serialPort->close()){
            //Если порт закрыт, блокировка элементов на форме
            ui->pB_ComOpen->setText("Открыть");
            ui->cB_ComList->setEnabled(true);
            ui->pB_config->setEnabled(false);
            ui->listV_F5->setEnabled(false);
            ui->listV_F6->setEnabled(false);
            ui->rButt_F0->setEnabled(false);
            ui->sBox_valueCAP->setEnabled(false);
            ui->pB_EnterCAP->setEnabled(false);
            ui->pB_sniffClear->setEnabled(false);
            ui->pB_reset->setEnabled(false);
            ui->pB_5361->setEnabled(false);
            ui->checkB_F1->setEnabled(false);
            ui->checkB_F2->setEnabled(false);
            ui->checkB_F3->setEnabled(false);
            ui->checkB_F4->setEnabled(false);
        }
        else emit errorSignal(" Не удалось закрыть порт");
    }
}

//Конфигурирование адаптера
void MainWindow::on_pB_config_clicked()
{
    serialPort->set_config();
    //ui->sBox_valueCAP->setValue(1);
}

//Обработка события выбора нового элемента в списке режимов
//args: index - индекс
void MainWindow::on_listV_F5_clicked(const QModelIndex &index)
{
    serialPort->func_10H_68(index.row(),5);
}

//Обработка события выбора нового элемента в списке поддиапазонов
//args: index - индекс
void MainWindow::on_listV_F6_clicked(const QModelIndex &index)
{
    serialPort->func_10H_68(index.row(),6);
}

//Обработка события нажатия радиокнопки вкл/выкл аттенюатора
//args: checked - режим аттенюатора
void MainWindow::on_rButt_F0_clicked(bool checked)
{
    if(checked){
        //аттенюатор включен - доп настройки доступны для изменений
        ui->checkB_F1->setEnabled(true);
        ui->checkB_F2->setEnabled(true);
        ui->checkB_F3->setEnabled(true);
        ui->checkB_F4->setEnabled(true);
    }
    else{
        //аттенюатор выключен - доп настройки недоступны для изменений
        ui->checkB_F1->setEnabled(false);
        ui->checkB_F2->setEnabled(false);
        ui->checkB_F3->setEnabled(false);
        ui->checkB_F4->setEnabled(false);
    }
    serialPort->func_10H_62(0);
}

//Обработка события нажатия кнопки сброса доп настроек аттенюатора
void MainWindow::on_pB_reset_clicked()
{
    ui->checkB_F1->setChecked(false);
    ui->checkB_F2->setChecked(false);
    ui->checkB_F3->setChecked(false);
    ui->checkB_F4->setChecked(false);

    serialPort->func_10H_62(14);
}

//Обработка событий выбора доп настроек аттенюатора
void MainWindow::on_checkB_F1_clicked()
{
    serialPort->func_10H_62(1);
}

void MainWindow::on_checkB_F2_clicked()
{
    serialPort->func_10H_62(2);
}

void MainWindow::on_checkB_F3_clicked()
{
    serialPort->func_10H_62(3);
}

void MainWindow::on_checkB_F4_clicked()
{
    serialPort->func_10H_62(4);
}

//Обработка события нажатия кнопки открытия/закрытия сниффера
void MainWindow::on_pB_sniffer_clicked()
{
    if((ui->pB_sniffer->text())==">>"){
            setMinimumSize(QSize(860,340));
            setMaximumSize(QSize(860,340));
            ui->pB_sniffer->setText("<<");
    }
    else{
            setMinimumSize(QSize(560,340));
            setMaximumSize(QSize(560,340));
            ui->pB_sniffer->setText(">>");
    }
}

//Обработка события нажатия кнопки очистки сниффера
void MainWindow::on_pB_sniffClear_clicked()
{
    ui->tE_sniffer->clear();
}

//Обработка сигнала добавления записи об операции в сниффер
//args: msg - байты отправленного/полученного пакета
void MainWindow::snifDataSlot(QString msg){

    ui->tE_sniffer->append(msg);
}

//Обработка события нажатия кнопки отправки значения ЦАП
void MainWindow::on_pB_EnterCAP_clicked()
{
    serialPort->func_6H(ui->sBox_valueCAP->value());
}

//Обработка события нажатия кнопки вкл/выкл генератора шума
void MainWindow::on_pB_5361_clicked()
{
    if((ui->pB_5361->text())=="Включен"){
        serialPort->func_10H_F5361(false);
        ui->pB_5361->setText("Выключен");

    }
    else{
        serialPort->func_10H_F5361(true);
        ui->pB_5361->setText("Включен");
    }
}

