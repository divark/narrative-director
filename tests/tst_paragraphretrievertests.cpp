/***********
 * Copyright (C) 2020 by Tyler Schmidt
 *
 *
 *   rolisteam is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *************/
#include "paragraphretriever.h"
#include <QtTest>

class ParagraphRetrieverTests : public QObject {
    Q_OBJECT

public:
    ParagraphRetrieverTests();
    ~ParagraphRetrieverTests();

private slots:
    void testGetNextSentenceNoQuote();
    void testGetNextSentenceInQuote();

    void testGetOnlyParagraph();
    void testGetFirstParagraph();
    void testGetSecondParagraph();
    void testGetNonExistantParagraph();
    void testGetCachedParagraph();

    void testGetParagraphCountWithOnlyOne();
    void testGetParagraphCountWithTwo();

private:
    QString firstParagraph = "This is a paragraph. It has four sentences. This "
                             "is the third! This is the fourth?";
    QString secondParagraph = "\"This is another paragraph.\" It could have a "
                              "sentence like this. Or maybe "
                              "like this? I would not know.";
    QString thirdParagraph =
        "    This is a paragraph. It has four sentences. This "
        "is the third! This is the fourth?";
    QString paragraphs = firstParagraph + "\n" + secondParagraph;
};

ParagraphRetrieverTests::ParagraphRetrieverTests() {}

ParagraphRetrieverTests::~ParagraphRetrieverTests() {}

void ParagraphRetrieverTests::testGetNextSentenceNoQuote() {
    ParagraphRetriever retriever(firstParagraph, 4);
    QString expectedSentence = "This is a paragraph.";
    QString actualSentence = retriever.getNextSentence().trimmed();

    QVERIFY2(expectedSentence.compare(actualSentence) == 0,
             qPrintable(QString("testGetNextSentenceNoQuote: "
                                "Mismatch between (%1) and (%2)")
                            .arg(expectedSentence)
                            .arg(actualSentence)));
}

void ParagraphRetrieverTests::testGetNextSentenceInQuote() {
    ParagraphRetriever retriever(secondParagraph, 4);
    QString expectedSentence = "\"This is another paragraph.\"";
    QString actualSentence = retriever.getNextSentence().trimmed();

    QVERIFY2(expectedSentence.compare(actualSentence) == 0,
             qPrintable(QString("testGetNextSentenceInQuote: "
                                "Mismatch between (%1) and (%2)")
                            .arg(expectedSentence)
                            .arg(actualSentence)));
}

void ParagraphRetrieverTests::testGetOnlyParagraph() {
    ParagraphRetriever retriever(firstParagraph, 4);
    QString firstRetrievedParagraph = retriever.getParagraph(0);

    QVERIFY2(firstRetrievedParagraph.compare(firstParagraph) == 0,
             qPrintable(QString("testGetOnlyParagraph: Mismatch between "
                                "expected paragraph of (%1) and (%2)")
                            .arg(firstParagraph)
                            .arg(firstRetrievedParagraph)));
}

void ParagraphRetrieverTests::testGetFirstParagraph() {
    ParagraphRetriever retriever(paragraphs, 4);
    QString firstRetrievedParagraph = retriever.getParagraph(0);

    QVERIFY2(firstRetrievedParagraph.compare(firstParagraph) == 0,
             qPrintable(QString("testGetFirstParagraph: Mismatch between "
                                "expected paragraph of (%1) and (%2)")
                            .arg(firstParagraph)
                            .arg(firstRetrievedParagraph)));
}

void ParagraphRetrieverTests::testGetSecondParagraph() {
    ParagraphRetriever retriever(paragraphs, 4);
    retriever.getParagraph(0);
    QString secondRetrievedParagraph = retriever.getParagraph(1);

    QVERIFY2(secondRetrievedParagraph.compare(secondParagraph) == 0,
             qPrintable(QString("testGetSecondParagraph: Mismatch between "
                                "expected paragraph of (%1) and (%2)")
                            .arg(secondParagraph)
                            .arg(secondRetrievedParagraph)));
}

void ParagraphRetrieverTests::testGetNonExistantParagraph() {
    ParagraphRetriever retriever(paragraphs, 4);
    retriever.getParagraph(0);
    retriever.getParagraph(1);
    QString nonExistantParagraph = retriever.getParagraph(2);

    QVERIFY(nonExistantParagraph == nullptr);
}

void ParagraphRetrieverTests::testGetCachedParagraph() {
    ParagraphRetriever retriever(paragraphs, 4);
    QString nonCachedParagraph = retriever.getParagraph(0);

    QVERIFY(nonCachedParagraph.compare(retriever.getParagraph(0)) == 0);
}

void ParagraphRetrieverTests::testGetParagraphCountWithOnlyOne() {
    ParagraphRetriever retriever(firstParagraph, 4);
    uint expectedNumPrgs = 1;
    uint actualNumPrgs = retriever.getNumParagraphs();

    QVERIFY(expectedNumPrgs == actualNumPrgs);
}

void ParagraphRetrieverTests::testGetParagraphCountWithTwo() {
    ParagraphRetriever retriever(paragraphs, 4);
    uint expectedNumPrgs = 2;
    uint actualNumPrgs = retriever.getNumParagraphs();

    QVERIFY(expectedNumPrgs == actualNumPrgs);
}

QTEST_APPLESS_MAIN(ParagraphRetrieverTests)

#include "tst_paragraphretrievertests.moc"
