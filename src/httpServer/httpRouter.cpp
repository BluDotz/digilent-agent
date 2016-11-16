//HTTP Controllers
#include "httpRouter.h"
#include "debugController.h"

StaticFileController* HttpRouter::staticFileController = 0;

HttpRouter::HttpRouter(QObject* _parent, OsDevice **_activeDevice) : HttpRequestHandler(_parent) {
    activeDevice = _activeDevice;
}

void HttpRouter::service(HttpRequest& request, HttpResponse& response) {

    //Connect slots and signals
    connect((*activeDevice), SIGNAL(execCommandComplete(QString)), this, SLOT(onComplete(QString)));
    connect(this, SIGNAL(deviceComplete()), &loop, SLOT(quit()));

    QByteArray path = request.getPath();
    QByteArray method = request.getMethod();

    qDebug() << "HttpRouter: method = " << method << "path = ", path.data();

    if(method == "OPTIONS"){
        response.setHeader("Access-Control-Allow-Headers", "Content-Type");
        response.setHeader("Access-Control-Allow-Origin", "*");
        response.setHeader("Access-Control-Max-Age", "86400");
        response.write("Options Response....", true);
    }
    else if (path=="/debug" || path=="/debug/") {
        qDebug("Routing To Debug Controller");
        DebugController(this).service(request, response);
    }
    else if (path=="/proxy" || path=="/proxy/") {
        waitingForResponse = true;
        (*activeDevice)->execCommand(request.getBody());

        //Wait for signal that device call has returned
        if(waitingForResponse){
            loop.exec();
        }

        //Add headers and return device call response to original requester
        response.setHeader("Access-Control-Allow-Origin", "*");
        response.setHeader("Cache-Control", "no-cache");
        response.setHeader("Connection", "close");
        response.setHeader("Content-Type", "application/json");
        response.setStatus(200, "OK");
        response.write(reply.toUtf8(), true);

        //Disconnect signals to prevent multiple responses on subsequent calls
        disconnect((*activeDevice), SIGNAL(execCommandComplete(QString)), this, SLOT(onComplete(QString)));
        //disconnect(this, SIGNAL(deviceComplete()), &loop, SLOT(quit()));
    }
    else{
        qDebug("Routing To Static Controller");
        staticFileController->service(request, response);
    }

    qDebug("HttpRouter: Request Complete");
}

void HttpRouter::onComplete(QString reply){
    qDebug("HttpRouter::onComplete()");
    waitingForResponse = false;
    this->reply = reply;
    emit deviceComplete();
}

