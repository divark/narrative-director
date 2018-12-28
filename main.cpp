#include "narrativedirector.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    NarrativeDirector w;
    w.show();

    return a.exec();
}
