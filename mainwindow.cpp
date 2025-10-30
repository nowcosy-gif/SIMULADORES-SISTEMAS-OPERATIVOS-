#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <algorithm>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QPushButton>
#include <QColor>
#include <QTextCharFormat>
#include <QRandomGenerator>
#include <QDebug>
#include <QTextCursor>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    // Se configura la tabla para 3 columnas unicamente: Proceso, T. Llegada, T. Rafaga
    ui->tableProcesos->setColumnCount(3);
    ui->tableProcesos->setHorizontalHeaderLabels({"Proceso", "T. Llegada", "T. Rafaga"});
    ui->tableProcesos->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    connect(ui->btnAgregar, &QPushButton::clicked, this, &MainWindow::agregarProceso);
    connect(ui->btnSimular, &QPushButton::clicked, this, &MainWindow::simularTodo);
    connect(ui->btnLimpiar, &QPushButton::clicked, this, &MainWindow::limpiarTodo);

    ui->txtResultados->setReadOnly(true);
    ui->txtGantt->setReadOnly(true);
    ui->txtGantt->setFont(QFont("Monospace", 10));

    agregarProceso();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::agregarProceso() {
    int row = ui->tableProcesos->rowCount();
    ui->tableProcesos->insertRow(row);

    // Se inicializan las 3 columnas
    ui->tableProcesos->setItem(row, 0, new QTableWidgetItem(QString("P%1").arg(row + 1)));
    ui->tableProcesos->setItem(row, 1, new QTableWidgetItem("0"));
    ui->tableProcesos->setItem(row, 2, new QTableWidgetItem("5"));
}

void MainWindow::limpiarTodo() {
    ui->tableProcesos->setRowCount(0);
    ui->txtResultados->clear();
    ui->txtGantt->clear();
    procesos.clear();
    resultados_sjf_ne.clear();
    resultados_srtf_e.clear();
    agregarProceso();
}

void MainWindow::simularTodo() {
    procesos.clear();
    int filas = ui->tableProcesos->rowCount();

    for (int i = 0; i < filas; ++i) {
        QTableWidgetItem *idItem = ui->tableProcesos->item(i, 0);
        QTableWidgetItem *llegadaItem = ui->tableProcesos->item(i, 1);
        QTableWidgetItem *rafagaItem = ui->tableProcesos->item(i, 2);

        if (!idItem || idItem->text().isEmpty() || !llegadaItem || llegadaItem->text().isEmpty() || !rafagaItem || rafagaItem->text().isEmpty()) {
            continue;
        }

        QString id = idItem->text();
        int llegada = llegadaItem->text().toInt();
        int rafaga = rafagaItem->text().toInt();

        Proceso p;
        p.nombre = id;
        p.llegada = llegada;
        p.rafaga = rafaga;
        p.rafagaRestante = rafaga;

        p.inicio_sjf_ne = p.fin_sjf_ne = p.espera_sjf_ne = p.retorno_sjf_ne = 0;
        p.inicio_srtf_e = p.fin_srtf_e = p.espera_srtf_e = p.retorno_srtf_e = -1;

        procesos.append(p);
    }

    if (procesos.isEmpty()) {
        QMessageBox::information(this, "Sin Procesos", "Anada procesos validos.");
        return;
    }

    resultados_sjf_ne = procesos;
    simularSJF_NoExpropiativo(resultados_sjf_ne);

    resultados_srtf_e = procesos;
    simularSRTF_Expropiativo(resultados_srtf_e);

    mostrarResultados();
    mostrarGantt_NoExpropiativo();
}

void MainWindow::simularSJF_NoExpropiativo(QVector<Proceso>& procesosInput) {
    QVector<Proceso> colaProcesos = procesosInput;
    QVector<Proceso> completados;
    int tiempoActual = 0;

    while (completados.size() < procesosInput.size()) {
        QVector<Proceso> disponibles;
        for (const auto& p : colaProcesos) {
            if (p.llegada <= tiempoActual) disponibles.append(p);
        }

        if (disponibles.isEmpty()) {
            int siguienteLlegada = -1;
            for (const auto& p : colaProcesos) {
                if (siguienteLlegada == -1 || p.llegada < siguienteLlegada) {
                    siguienteLlegada = p.llegada;
                }
            }
            if (siguienteLlegada != -1 && siguienteLlegada > tiempoActual) {
                tiempoActual = siguienteLlegada;
                continue;
            } else {
                break;
            }
        }

        std::sort(disponibles.begin(), disponibles.end(), [](const Proceso& a, const Proceso& b) {
            if (a.rafaga != b.rafaga) return a.rafaga < b.rafaga;
            return a.llegada < b.llegada;
        });

        Proceso procesoSeleccionado = disponibles.first();

        for (int i = 0; i < procesosInput.size(); ++i) {
            if (procesosInput[i].nombre == procesoSeleccionado.nombre) {
                procesosInput[i].inicio_sjf_ne = tiempoActual;
                procesosInput[i].fin_sjf_ne = tiempoActual + procesosInput[i].rafaga;
                procesosInput[i].retorno_sjf_ne = procesosInput[i].fin_sjf_ne - procesosInput[i].llegada;
                procesosInput[i].espera_sjf_ne = procesosInput[i].retorno_sjf_ne - procesosInput[i].rafaga;

                tiempoActual = procesosInput[i].fin_sjf_ne;
                completados.append(procesosInput[i]);

                colaProcesos.erase(std::remove_if(colaProcesos.begin(), colaProcesos.end(),
                                                  [&](const Proceso& p) { return p.nombre == procesosInput[i].nombre; }),
                                   colaProcesos.end());
                break;
            }
        }
    }
    procesosInput = completados;
}

void MainWindow::simularSRTF_Expropiativo(QVector<Proceso>& procesosInput) {
    QVector<Proceso> colaTrabajo = procesosInput;
    QVector<Proceso> completados;

    for(auto& p : colaTrabajo) {
        p.rafagaRestante = p.rafaga;
        p.espera_srtf_e = 0;
        p.inicio_srtf_e = -1;
    }

    int tiempoActual = 0;
    int procesosRestantes = colaTrabajo.size();

    while (procesosRestantes > 0) {
        int indiceMenor = -1;
        int minRestante = -1;

        for (int i = 0; i < colaTrabajo.size(); ++i) {
            if (colaTrabajo[i].llegada <= tiempoActual && colaTrabajo[i].rafagaRestante > 0) {

                if (indiceMenor == -1 || colaTrabajo[i].rafagaRestante < minRestante) {
                    minRestante = colaTrabajo[i].rafagaRestante;
                    indiceMenor = i;
                }
                else if (colaTrabajo[i].rafagaRestante == minRestante && colaTrabajo[i].llegada < colaTrabajo[indiceMenor].llegada) {
                    minRestante = colaTrabajo[i].rafagaRestante;
                    indiceMenor = i;
                }
            }
        }

        if (indiceMenor == -1) {
            tiempoActual++;
            continue;
        }

        Proceso& actual = colaTrabajo[indiceMenor];

        if (actual.inicio_srtf_e == -1) {
            actual.inicio_srtf_e = tiempoActual;
        }

        actual.rafagaRestante--;

        for (int i = 0; i < colaTrabajo.size(); ++i) {
            if (i != indiceMenor && colaTrabajo[i].llegada <= tiempoActual && colaTrabajo[i].rafagaRestante > 0) {
                colaTrabajo[i].espera_srtf_e++;
            }
        }

        tiempoActual++;

        if (actual.rafagaRestante == 0) {
            procesosRestantes--;

            actual.fin_srtf_e = tiempoActual;
            actual.retorno_srtf_e = actual.fin_srtf_e - actual.llegada;

            completados.append(actual);
        }
    }

    procesosInput = completados;
}


void MainWindow::mostrarResultados() {
    QString html = "";

    // Texto introductorio SRTF (sin fondo)
    html += "<p style='font-size:10pt; border:1px solid #cce5ff; padding:5px; margin-bottom: 10px;'>";
    html += "<strong>EN ESTE RECUADRO SE PRESENTA EL ANALISIS SRTF COMO UN EXTRA:</strong> debido a que la mayoria de ejercicios resuelven primero SRTF (SJF Expropiativo) y despues SJF (SJF No Expropiativo) para comparacion.";
    html += "</p>";

    // Resultados SJF EXPROPIATIVO (SRTF)
    html += "<h3 style='color:#0056b3; margin-top: 15px;'>Analisis y Tiempos de Ejecucion: SJF Expropiativo (SRTF)</h3>";
    html += "<style>table {border-collapse: collapse; width: 100%; font-size: 10pt;} th, td {border: 1px solid black; padding: 4px; text-align: left;} th {background-color: #e0f7fa;}</style>";
    html += "<table><tr><th>Proceso</th><th>T. Llegada</th><th>T. Rafaga</th><th>T. Inicio</th><th>T. Fin</th><th>T. Espera</th><th>T. Respuesta</th></tr>";

    double totalEsperaSRTF = 0;
    double totalRetornoSRTF = 0;

    std::sort(resultados_srtf_e.begin(), resultados_srtf_e.end(), [](const Proceso& a, const Proceso& b) { return a.nombre < b.nombre; });

    for (const auto& p : resultados_srtf_e) {
        html += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td><td>%7</td></tr>")
                    .arg(p.nombre).arg(p.llegada).arg(p.rafaga).arg(p.inicio_srtf_e).arg(p.fin_srtf_e).arg(p.espera_srtf_e).arg(p.retorno_srtf_e);
        totalEsperaSRTF += p.espera_srtf_e;
        totalRetornoSRTF += p.retorno_srtf_e;
    }
    html += "</table><br>";

    if (!resultados_srtf_e.isEmpty()) {
        html += QString("<h4>Promedios SRTF:</h4>");
        html += QString("T. Espera Promedio: <strong>%1</strong><br>").arg(totalEsperaSRTF / resultados_srtf_e.size(), 0, 'f', 2);
        html += QString("T. Respuesta Promedio: <strong>%1</strong><br>").arg(totalRetornoSRTF / resultados_srtf_e.size(), 0, 'f', 2);
    }

    // Resultados SJF NO EXPROPIATIVO
    html += "<br><h3 style='color:#38761d;'>Analisis y Tiempos de Ejecucion: SJF No Expropiativo</h3>";
    html += "<style>table {border-collapse: collapse; width: 100%; font-size: 10pt;} th, td {border: 1px solid black; padding: 4px; text-align: left;} th {background-color: #d9ead3;}</style>";
    html += "<table><tr><th>Proceso</th><th>T. Llegada</th><th>T. Rafaga</th><th>T. Inicio</th><th>T. Fin</th><th>T. Espera</th><th>T. Respuesta</th></tr>";

    double totalEsperaSJF = 0;
    double totalRetornoSJF = 0;

    std::sort(resultados_sjf_ne.begin(), resultados_sjf_ne.end(), [](const Proceso& a, const Proceso& b) { return a.nombre < b.nombre; });

    for (const auto& p : resultados_sjf_ne) {
        html += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td><td>%7</td></tr>")
                    .arg(p.nombre).arg(p.llegada).arg(p.rafaga).arg(p.inicio_sjf_ne).arg(p.fin_sjf_ne).arg(p.espera_sjf_ne).arg(p.retorno_sjf_ne);
        totalEsperaSJF += p.espera_sjf_ne;
        totalRetornoSJF += p.retorno_sjf_ne;
    }
    html += "</table><br>";

    if (!resultados_sjf_ne.isEmpty()) {
        html += QString("<h4>Promedios SJF No Expropiativo:</h4>");
        html += QString("T. Espera Promedio: <strong>%1</strong><br>").arg(totalEsperaSJF / resultados_sjf_ne.size(), 0, 'f', 2);
        html += QString("T. Respuesta Promedio: <strong>%1</strong><br>").arg(totalRetornoSJF / resultados_sjf_ne.size(), 0, 'f', 2);
    }

    ui->txtResultados->setHtml(html);
}

void MainWindow::formatearGantt(const QVector<Proceso>& procesosGantt) {
    ui->txtGantt->clear();

    // Texto introductorio (sin fondo gris)
    ui->txtGantt->append(
        "<p style='color:black; font-weight:bold; padding:5px; margin-bottom: 5px; background-color: transparent;'>EN ESTE CUADRO SE PRESENTA EL DIAGRAMA DE GANTT PARA SJF NO EXPROPIATIVO</p>"
        );

    // Dos espacios abajo
    ui->txtGantt->append("\n\n");

    QTextCharFormat fmt;
    int tiempoActual = 0;

    // Colores
    QMap<QString, QColor> colores;
    QVector<QColor> coloresBase = {
        QColor("#e74c3c"),
        QColor("#3498db"),
        QColor("#2ecc71"),
        QColor("#f1c40f"),
        QColor("#9b59b6"),
        QColor("#1abc9c")
    };

    int colorIndex = 0;
    for (const auto& p : procesosGantt) {
        if (!colores.contains(p.nombre)) {
            colores[p.nombre] = coloresBase[colorIndex++ % coloresBase.size()];
        }
    }

    QTextCursor cursor(ui->txtGantt->document());
    cursor.movePosition(QTextCursor::End);

    // Dibujo del Gantt
    for (const auto &p : procesosGantt) {
        int inicio = p.inicio_sjf_ne;
        int fin = p.fin_sjf_ne;

        // Espacio inactivo (transparente)
        if (inicio > tiempoActual) {
            fmt.setForeground(QColor("black"));
            fmt.setBackground(QColor("transparent"));
            fmt.setFontWeight(QFont::Normal);
            fmt.setFontPointSize(10);
            cursor.setCharFormat(fmt);
            cursor.insertText(QString("[%1-%2] Inactivo ").arg(tiempoActual).arg(inicio));
        }

        // Bloque de proceso (negrita y grande)
        QColor colorProceso = colores.value(p.nombre, QColor("black"));
        fmt.setForeground(QColor("white"));
        fmt.setBackground(colorProceso);
        fmt.setFontWeight(QFont::Bold);
        fmt.setFontPointSize(12);
        cursor.setCharFormat(fmt);
        cursor.insertText(QString("[%1-%2] %3 ").arg(inicio).arg(fin).arg(p.nombre));

        tiempoActual = fin;
    }

    // Restaurar formato
    fmt.setBackground(QColor("white"));
    fmt.setForeground(QColor("black"));
    fmt.setFontWeight(QFont::Normal);
    fmt.setFontPointSize(10);
    cursor.setCharFormat(fmt);
}

void MainWindow::mostrarGantt_NoExpropiativo() {
    QVector<Proceso> procesosGantt = resultados_sjf_ne;
    std::sort(procesosGantt.begin(), procesosGantt.end(), [](const Proceso& a, const Proceso& b) {
        return a.inicio_sjf_ne < b.inicio_sjf_ne;
    });
    formatearGantt(procesosGantt);
}
