#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QStringListModel>
#include <QKeyEvent>
#include "serial_port.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    //Вывод сообщения об ошибке на форму
    void errorSlot(QString);

    //Вывод сообщения об успешной операции на форму
    void successSlot(QString);

    //Сброс информационного сообщения на форме
    void inactiveSlot();

    //Открытие/закрытие порта
    void on_pB_ComOpen_clicked();

    //Конфигурирование адаптера
    void on_pB_config_clicked();

    //Обработка сигнала добавления записи об операции в сниффер
    void snifDataSlot(QString);

    //Визуальное выделение элемента в списке режимов
    void F5_SelectSlot(int);

    //Визуальное выделение элемента в списке поддиапазонов
    void F6_SelectSlot(int);

    //Вывод сообщения об отсутствии ответа на форму
    void noAnswSlot();

    //Визуальный эффект отправки данных на форме
    void borderColorSlot();

    //Сброс визуального эффекта отправки данных
    void borderInactSlot();

    //Обработка события выбора нового элемента в списке режимов
    void on_listV_F5_clicked(const QModelIndex &index);

    //Обработка события выбора нового элемента в списке поддиапазонов
    void on_listV_F6_clicked(const QModelIndex &index);

    //Обработка события нажатия радиокнопки вкл/выкл аттенюатора
    void on_rButt_F0_clicked(bool checked);

    //Обработка события нажатия кнопки сброса доп настроек аттенюатора
    void on_pB_reset_clicked();

    //Обработка событий выбора доп настроек аттенюатора
    void on_checkB_F1_clicked();
    void on_checkB_F2_clicked();
    void on_checkB_F3_clicked();
    void on_checkB_F4_clicked();

    //Обработка события нажатия кнопки открытия/закрытия сниффера
    void on_pB_sniffer_clicked();

    //Обработка события нажатия кнопки очистки сниффера
    void on_pB_sniffClear_clicked();

    //Обработка события нажатия кнопки отправки значения ЦАП
    void on_pB_EnterCAP_clicked();

    //Обработка события нажатия кнопки вкл/выкл генератора шума
    void on_pB_5361_clicked();

    //Установка значения ЦАП визуально на форме
    void valueCAPSlot(int);

protected:
    void keyReleaseEvent(QKeyEvent *key);

private:
    Ui::MainWindow *ui;

    //Экземпляр класса для формирования пакетов и работы с COM-портом
    SerialPort* serialPort;

signals:
    //Сигнал с сообщением об ошибке для отображения на форме
    errorSignal(QString);
};

#endif // MAINWINDOW_H
