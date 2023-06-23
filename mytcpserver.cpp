#include <mytcpserver.h>
#include <qdebug.h>
#include <qexception.h>

MyTcpServer::~MyTcpServer() {
  this->close();
  server_status = false;
}

MyTcpServer::MyTcpServer(QObject *parent) : QTcpServer(parent) {
  connect(this, &QTcpServer::newConnection, this,
          &MyTcpServer::slotNewConnection);
  initDB();
  if (!this->listen(QHostAddress::Any, 33333)) {
    qDebug() << "server is not started";
  } else {
    server_status = true;
    qDebug() << "server is started";
  }
}
void MyTcpServer::initDB() {
  db = QSqlDatabase::addDatabase("QSQLITE");
  db.setDatabaseName("mydb.db");

  if (!db.open()) {
    qDebug() << "Error opening database: " << db.lastError().text();
  }
  QSqlQuery query(db);
  QString queryString = " DROP TABLE IF EXISTS users; ";
  if (!query.exec(queryString)) {
    qDebug() << "Error executing query:" << query.lastError().text();
  }
  queryString =
      "CREATE TABLE IF NOT EXISTS users ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "login TEXT,"
      "currentNumber INT, lossCount INT, winCount INT,"
      "port INT"
      ");";
  if (!query.exec(queryString)) {
    qDebug() << "Error executing query:" << query.lastError().text();
  }
}

void MyTcpServer::slotNewConnection() {
  if (server_status == 1) {
    QTcpSocket *currTcpSocket;

    currTcpSocket = this->nextPendingConnection();
    currTcpSocket->write("Connected to server\r\n");
    connect(currTcpSocket, &QTcpSocket::readyRead, this,
            &MyTcpServer::slotServerRead);
    connect(currTcpSocket, &QTcpSocket::disconnected, this,
            &MyTcpServer::slotClientDisconnected);
    tcpSockets.push_back(currTcpSocket);
  }
}

void MyTcpServer::slotServerRead() {
  QByteArray array;
  QTcpSocket *currTcpSocket = (QTcpSocket *)sender();
  while (currTcpSocket->bytesAvailable() > 0) {
    array.append(currTcpSocket->readAll());
  }
  if (array.right(1) == "\n") {
    currTcpSocket->write(parse(array.trimmed(), currTcpSocket));
  }
}

void MyTcpServer::slotClientDisconnected() {
  QTcpSocket *currTcpSocket = (QTcpSocket *)sender();
  currTcpSocket->close();
}

QByteArray MyTcpServer::parse(QByteArray array, QTcpSocket *currTcpSocket) {
  QString data = QString::fromUtf8(array).trimmed();
  QStringList commandParts = QString::fromUtf8(array).trimmed().split('&');
  quint16 port = currTcpSocket->peerPort();
  if (commandParts.isEmpty()) {
    return "Неправильная команда\n";
  }

  QString command = commandParts.first();

  if (command == "start" && commandParts.size() == 2) {
    QString login = commandParts.at(1);
    qDebug() << port;
    return addNewUser(login, port);
  } else if (command == "break" && commandParts.size() == 1) {
    deleteUser(port);
    return "\n";
  } else if (command == "stats" && commandParts.size() == 1) {
    return stats(port);
  } else if (command == "choice" && commandParts.size() == 2) {
    QString number = commandParts.at(1);
    int numberInt = number.toInt();
    return choice(numberInt, port);
  } else {
    return "Неправильная команда\n";
  }
}

QByteArray MyTcpServer::addNewUser(QString login, int port) {
  QSqlQuery query(db);

  QString checkUserCountQuery = "SELECT COUNT(*) AS userCount FROM users; ";
  query.prepare(checkUserCountQuery);
  if (!query.exec()) {
    qDebug() << "Error executing query:" << query.lastError().text();
    return "Ошибка при выполнении команды start\n";
  }
  if (query.next()) {
    int userCount = query.value("userCount").toInt();
    if (userCount < 5) {
      QString queryString =
          "INSERT INTO users (login, port, lossCount, winCount) "
          "VALUES (:login, :port, :lossCount, :winCount);";
      query.prepare(queryString);
      query.bindValue(":login", login);
      query.bindValue(":port", port);
      query.bindValue(":lossCount", 0);
      query.bindValue(":winCount", 0);
      if (query.exec()) {
        if (userCount == 4) {
          chekUsers();
          return "\n";
        }
        return "waiting\n";
      } else {
        qDebug() << "Error executing query:" << query.lastError().text();
        return "Ошибка при выполнении команды start\n";
      }
    } else {
      return "Комната заполнена\n";
    }
  } else {
    qDebug() << "Error executing query:" << query.lastError().text();
    return "Ошибка при выполнении команды start\n";
  }
}

QByteArray MyTcpServer::stats(int port) {
  QSqlQuery query(db);
  QString queryString =
      "SELECT users.login "
      "FROM users;";
  query.prepare(queryString);
  query.bindValue(":port", port);
  if (query.exec()) {
    QString logins = "Очередь:\n";
    while (query.next()) {
      QString login = query.value("login").toString();
      logins += login + "\n";
    }
    return logins.toUtf8();
  } else {
    qDebug() << "Error executing query:" << query.lastError().text();
    return "Ошибка при выполнении команды stats\n";
  }
}

void MyTcpServer::chekUsers() {
  QSqlQuery query(db);
  int userCount = 0;
  QString checkUserCountQuery = "SELECT COUNT(*) AS userCount FROM users; ";
  query.prepare(checkUserCountQuery);
  if (!query.exec()) {
    qDebug() << "Error executing query:" << query.lastError().text();
  }
  if (query.next()) {
    userCount = query.value("userCount").toInt();
  }
  if (userCount == 5) {
    choiceStatus = true;
    QString checkUserCountQuery = "SELECT port FROM users; ";
    query.prepare(checkUserCountQuery);
    if (!query.exec()) {
      qDebug() << "Error executing query:" << query.lastError().text();
    }
    QList<int> ports;
    while (query.next()) {
      int port = query.value("port").toInt();
      ports.append(port);
    }
    sendAll(ports, "make_move\n");
  }
}

void MyTcpServer::sendAll(QList<int> ports, QString message) {
  for (QTcpSocket *socket : tcpSockets) {
    int socketPort = socket->peerPort();
    if (ports.contains(socketPort)) {
      socket->write(message.toUtf8());
    }
  }
}

void MyTcpServer::checkNumbers() {
  QSqlQuery query(db);
  bool flag = true;
  QString queryString =
      "SELECT port, currentNumber "
      "FROM users;";
  query.prepare(queryString);

  if (query.exec()) {
    QList<int> winPorts;
    QList<int> lossPorts;

    while (query.next()) {
      int port = query.value("port").toInt();
      int currentNumber = query.value("currentNumber").toInt();

      if (currentNumber == -999) {
        flag = false;
      } else {
        if (currentNumber == checkNumber()) {
          winPorts.append(port);
        } else {
          lossPorts.append(port);
        }
      }
    }
    if (flag) {
      sendAll(winPorts, "Win\n");
      sendAll(lossPorts, "Loss\n");
    }
    for (int port : winPorts) {
      queryString =
          "UPDATE users SET winCount = winCount + 1, currentNumber = NULL "
          "WHERE port = : port;";
      query.prepare(queryString);
      query.bindValue(":port", port);
      if (!query.exec()) {
        qDebug() << "Error executing query:" << query.lastError().text();
      }
      breakConnection(port);
    }

    for (int port : lossPorts) {
      queryString =
          "UPDATE users SET lossCount = lossCount + 1, currentNumber = NULL "
          "WHERE port = : port;";
      query.prepare(queryString);
      query.bindValue(":port", port);
      if (!query.exec()) {
        qDebug() << "Error executing query:" << query.lastError().text();
      }
      breakConnection(port);
    }

  } else {
    qDebug() << "Error executing query:" << query.lastError().text();
  }
}

int MyTcpServer::checkNumber() {
  QSqlQuery query(db);
  QString queryString =
      "SELECT currentNumber "
      "FROM users;";
  query.prepare(queryString);
  if (query.exec()) {
    int number = 0;
    while (query.next()) {
      int currNumber = query.value("currentNumber").toInt();
      if (currNumber == 0) {
        return 0;
      } else if (currNumber > number) {
        number = currNumber;
      }
    }
    return number;
  } else {
    qDebug() << "Error executing query:" << query.lastError().text();
    return -1;
  }
}

QByteArray MyTcpServer::choice(int number, int port) {
  QSqlQuery query(db);
  QString queryString =
      "UPDATE users SET currentNumber = :number "
      "WHERE port = :port;";
  query.prepare(queryString);
  query.bindValue(":number", number);
  query.bindValue(":port", port);
  if (!query.exec()) {
    qDebug() << "Error executing query:" << query.lastError().text();
    return "Ошибка при выполнении команды choice\n";
  } else {
    choices++;
    if (choices == 5) {
      checkNumbers();
    }
    return "\n";
  }
}
void MyTcpServer::breakConnection(int port) {
  for (QTcpSocket *socket : tcpSockets) {
    int socketPort = socket->peerPort();
    if (socketPort == port) {
      socket->close();
    }
  }
}

void MyTcpServer::deleteUser(int port) {
  QSqlQuery query(db);
  QString queryString =
      "DELETE FROM users "
      "WHERE port = :port;";
  query.prepare(queryString);
  query.bindValue(":port", port);
  if (!query.exec()) {
    qDebug() << "Error executing query:" << query.lastError().text();
  }
  breakConnection(port);
}
