#include "paragraphretriever.h"

ParagraphRetriever::ParagraphRetriever(QFile *textFile, uint sentenceLimit) {
    QTextStream textStream(textFile);

    ParagraphRetriever(textStream.readAll(), sentenceLimit);
}

ParagraphRetriever::ParagraphRetriever(const QString &text,
                                       uint sentenceLimit) {
    this->textFileContents = text;
    this->sentenceFinder =
        QTextBoundaryFinder(QTextBoundaryFinder::Sentence, textFileContents);
    this->sentenceLimit = sentenceLimit;
    getNumParagraphs();
}

QString ParagraphRetriever::getParagraph(int paragraphNum) {
    if (knownParagraphs.length() > paragraphNum) {
        return knownParagraphs[paragraphNum].second;
    }

    auto sentenceFinderPos = sentenceFinder.position();
    if (sentenceFinderPos == textFileContents.length()) {
        return nullptr;
    }

    QString paragraph = generateParagraphFromSentences();
    knownParagraphs.append(std::make_pair(sentenceFinderPos, paragraph));

    return paragraph;
}

QString ParagraphRetriever::generateParagraphFromSentences() {
    QString paragraph = "";
    for (uint i = 0; i < sentenceLimit; i++) {
        paragraph.append(getNextSentence());
    }

    return paragraph.trimmed();
}

QString ParagraphRetriever::getNextSentence() {
    auto currentSentencePosition = sentenceFinder.position();
    auto nextSentenceLocation = sentenceFinder.toNextBoundary();
    auto sentenceLength = nextSentenceLocation - currentSentencePosition;

    QString sentence =
        textFileContents.mid(currentSentencePosition, sentenceLength);

    return sentence;
}

uint ParagraphRetriever::getNumParagraphs() {
    if (numPrgs > 0) {
        return numPrgs;
    }

    uint numSentences = 0;

    while (sentenceFinder.toNextBoundary() != -1) {
        numSentences++;
    }

    sentenceFinder.toStart();
    numPrgs = ceil((float)numSentences / (float)sentenceLimit);

    return numPrgs;
}

void ParagraphRetriever::setPosition(uint position) {
    sentenceFinder.setPosition(position);
}
