#ifndef PARAGRAPHRETRIEVER_H
#define PARAGRAPHRETRIEVER_H

#include <QFile>
#include <QTextBoundaryFinder>
#include <QTextStream>
#include <cmath>

class ParagraphRetriever {
public:
    ParagraphRetriever(QFile *, uint);
    ParagraphRetriever(const QString &, uint);

    QString getParagraph(int);
    QString getNextSentence();
    uint getNumParagraphs();

    void setPosition(uint);

private:
    QString textFileContents;
    QTextBoundaryFinder sentenceFinder;
    uint sentenceLimit = 0;

    QVector<std::pair<uint, QString>> knownParagraphs;
    uint numPrgs = 0;

    QString generateParagraphFromSentences();
};

#endif // PARAGRAPHRETRIEVER_H
