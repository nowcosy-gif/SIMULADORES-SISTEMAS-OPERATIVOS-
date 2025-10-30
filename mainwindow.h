#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void agregarProceso();
    void simularTodo();
    void limpiarTodo();

private:
    Ui::MainWindow *ui;

    struct Proceso {
        QString nombre;
        int llegada;
        int rafaga;
        int rafagaRestante; // Para SRTF

        // Resultados SJF No Expropiativo (ne)
        int inicio_sjf_ne;
        int fin_sjf_ne;
        int espera_sjf_ne;
        int retorno_sjf_ne;

        // Resultados SRTF Expropiativo (e)
        int inicio_srtf_e;
        int fin_srtf_e;
        int espera_srtf_e;
        int retorno_srtf_e;
    };

    QVector<Proceso> procesos;
    QVector<Proceso> resultados_sjf_ne;
    QVector<Proceso> resultados_srtf_e;

    // Funciones de simulacion y presentacion
    void simularSJF_NoExpropiativo(QVector<Proceso>& procesosInput);
    void simularSRTF_Expropiativo(QVector<Proceso>& procesosInput);
    void mostrarResultados();
    void mostrarGantt_NoExpropiativo();
    void formatearGantt(const QVector<Proceso>& procesosGantt);
};

#endif // MAINWINDOW_H
