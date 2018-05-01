/* This file is part of the Calligra libraries
   Copyright (C) 2001 Werner Trobin <trobin@kde.org>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public License
along with this library; see the file COPYING.LIB.  If not, write to
the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
Boston, MA 02110-1301, USA.
*/

#ifndef KIS_FILTER_CHAIN_H
#define KIS_FILTER_CHAIN_H

#include <QHash>
#include <QList>
#include <QStringList>

#include <KoStoreDevice.h>
#include "kis_shared_ptr.h"

#include "KisFilterEntry.h"
#include "KisImportExportFilter.h"
#include "KisFilterChainLinkList.h"

#include "kis_shared.h"

#include "kritaui_export.h"


class QTemporaryFile;
class KisImportExportManager;
class KisDocument;

namespace CalligraFilter
{
    class Graph;
    class ChainLink;
    class Vertex;
    class Edge;
}

/**
 * @brief This class represents a chain of plain @ref KisImportExportFilter instances.
 *
 * @author Werner Trobin <trobin@kde.org>
 * @todo the class has no constructor and therefore cannot initialize its private class
 */
class KRITAUI_EXPORT KisFilterChain : public KisShared
{
public:

    explicit KisFilterChain(const KisImportExportManager *manager);
    virtual ~KisFilterChain();

    /**
     * The filter manager returned may be 0!
     */
    const KisImportExportManager* manager() const {
        return m_manager;
    }

    /**
     * Starts the filtering process.
     * @return The return status of the conversion. KisImportExportFilter::OK
     * if everything is alright.
     */
    KisImportExportFilter::ConversionStatus invokeChain();

    /**
     * Tells the @ref KisFilterManager the output file of the
     * filter chain in case of an import operation. If it's
     * an empty QString we directly manipulated the document.
     */
    QString chainOutput() const;

    /**
     * Get the current file to read from. This part of the API
     * is for the filters in our chain.
     */
    QString inputFile();

    /**
     * Get the current file to write to. This part of the API
     * is for the filters in our chain.
     */
    QString outputFile();
    void setOutputFile(const QString &outputFile);

    /**
     * This method allows your filter to work directly on the
     * @ref KisDocument of the application.
     * This part of the API is for the filters in our chain.
     * @return The document containing the data. May return 0 on error.
     */
    KisDocument *inputDocument();

    /**
     * This method allows your filter to work directly on the
     * @ref KisDocument of the application.
     * This part of the API is for the filters in our chain.
     * @return The document you have to write to. May return 0 on error.
     */
    KisDocument *outputDocument();

    KisPropertiesConfigurationSP filterManagerExportConfiguration() const;

    /// returns the amount of filters this chain contains representing the weight
    int weight() const;

private:
    // ### API for Calligra::Graph:
    // Construct a filter chain belonging to some KisFilterManager.
    // The parent filter manager may be 0.

    friend class CalligraFilter::Graph;

    void prependChainLink(KisFilterEntrySP filterEntry, const QByteArray& from, const QByteArray& to);

    // These methods are friends of KisFilterManager and provide access
    // to a private part of its API. As I don't want to include
    // koFilterManager.h in this header the direction is "int" here.

    friend class KisImportExportManager;

    QString filterManagerImportFile() const;
    QString filterManagerExportFile() const;
    KisDocument* filterManagerKisDocument() const;
    int filterManagerDirection() const;

    // Helper methods which keep track of all the temp files and documents,
    // and properly delete them as soon as they are not
    // needed anymore.
    void manageIO();
    void finalizeIO();

    bool createTempFile(QTemporaryFile** tempFile, bool autoDelete = true);

    void inputFileHelper(KisDocument* document, const QString& alternativeFile);
    void outputFileHelper(bool autoDelete);

    // "A whole is that which has beginning, middle, and end" - Aristotle
    // ...but we also need to signal "Done" state, Mr. Aristotle
    enum Whole { Beginning = 1, Middle = 2, End = 4, Done = 8 };

    // Don't copy or assign filter chains
    KisFilterChain(const KisFilterChain& rhs);
    KisFilterChain& operator=(const KisFilterChain& rhs);

    const KisImportExportManager* const m_manager;

    CalligraFilter::ChainLinkList m_chainLinks;

    // stuff needed for bookkeeping
    int m_state;

    QString m_inputFile;              // Did we pass around plain files?
    QString m_outputFile;

    KisDocument* m_inputDocument;      // ...or even documents?
    KisDocument* m_outputDocument;

    QTemporaryFile* m_inputTempFile;
    QTemporaryFile* m_outputTempFile;

    // These two flags keep track of the input/output the
    // filter (=user) asked for
    enum IOState { Nil, File, Document };
    IOState m_inputQueried, m_outputQueried;

    class Private;
    Private * const d;
};

#endif // __KO_FILTER_CHAIN_H__
