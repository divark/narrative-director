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
