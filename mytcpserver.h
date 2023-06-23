#pragma once
#include <QByteArray>
#include <QDebug>
#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtNetwork>
class MyTcpServer : public QTcpServer {
  Q_OBJECT
 public:
  explicit MyTcpServer(QObject *parent = nullptr);
  ~MyTcpServer();
  QByteArray parse(QByteArray array, QTcpSocket *currTcpSocket);
  void initDB();
  QByteArray addNewUser(QString login, int port);
  void breakConnection(int port);
  QByteArray stats(int port);
  QByteArray choice(int number, int port);
  int checkNumber();
  void checkNumbers();
  void sendAll(QList<int> ports, QString message);
  void chekUsers();
  void deleteUser(int port);

 public slots:
  void slotNewConnection();
  void slotClientDisconnected();
  void slotServerRead();

 private:
  QTcpServer *tcpServer;
  QVector<QTcpSocket *> tcpSockets;
  int server_status;
  QSqlDatabase db;
  bool choiceStatus = false;
  int choices = 0;
};