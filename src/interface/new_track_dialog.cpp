/*
* The MIT License (MIT)
*
* IziEditor
* Copyright (c) 2015 Martin Newhouse
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#include "new_track_dialog.hpp"
#include "track_essentials.hpp"

#include <qfiledialog.h>
#include <qfile.h>

namespace interface
{
    NewTrackDialog::NewTrackDialog(QWidget* parent)
        : QDialog(parent)
    {
        widgets_.setupUi(this);

        connect(widgets_.widthSlider, SIGNAL(valueChanged(int)), widgets_.widthSpinBox, SLOT(setValue(int)));
        connect(widgets_.heightSlider, SIGNAL(valueChanged(int)), widgets_.heightSpinBox, SLOT(setValue(int)));

        connect(widgets_.widthSpinBox, SIGNAL(valueChanged(int)), widgets_.widthSlider, SLOT(setValue(int)));
        connect(widgets_.heightSpinBox, SIGNAL(valueChanged(int)), widgets_.heightSlider, SLOT(setValue(int)));

        connect(widgets_.browseLocation, SIGNAL(clicked()), this, SLOT(browse_locations()));
        connect(widgets_.confirmButton, SIGNAL(accepted()), this, SLOT(confirm_track_creation()));
        connect(widgets_.confirmButton, SIGNAL(rejected()), this, SLOT(cancel_track_creation()));

        connect(widgets_.trackName, SIGNAL(textChanged(const QString&)), this, SLOT(validate_track_name(const QString&)));
    }

    void NewTrackDialog::browse_locations()
    {
        QString location = QFileDialog::getExistingDirectory(this, "Track Location", widgets_.trackLocation->text());

        widgets_.trackLocation->setText(location);
    }

    void NewTrackDialog::confirm_track_creation()
    {
        QString track_name = widgets_.trackName->text();
        QString track_author = widgets_.author->text();

        if (track_name.isEmpty())
        {
            widgets_.trackName->setFocus();
        }

        else if (track_author.isEmpty())
        {
            widgets_.author->setFocus();
        }

        else
        {
            TrackEssentials essentials;
            essentials.path = QDir::cleanPath(widgets_.trackLocation->text() +
                QDir::separator() + track_name + ".trk");

            essentials.name = track_name;
            essentials.author = track_author;

            essentials.size.setWidth(widgets_.widthSpinBox->value());
            essentials.size.setHeight(widgets_.heightSpinBox->value());

            for (int row = 0; row != widgets_.assetsList->count(); ++row)
            {
                QString asset = widgets_.assetsList->item(row)->text();
                essentials.assets.push_back(asset);
            }

            create_track(essentials);

            hide();
        }
    }

    void NewTrackDialog::cancel_track_creation()
    {
        hide();
    }

    void NewTrackDialog::validate_track_name(const QString& text)
    {
        QString validated;
        validated.reserve(text.size());

        auto is_valid = [](QChar ch)
        {
            // Because Turbo Sliders only supports a very basic character set, that's
            // what we're going to do here as well.
            char narrow_ch = ch.toLatin1();

            switch (narrow_ch)
            {
            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h':
            case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
            case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x':
            case 'y': case 'z':
                
            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
            case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P':
            case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': 
            case 'Y': case 'Z':

            case '0': case '1': case '2': case '3': case '4': 
            case '5': case '6': case '7': case '8': case '9':

            case '_': case '-': case ' ':  case '\'': case ',':
            case '(': case ')': case '[': case ']': case '!': case '+':
                return true;

            default:
                return false;
            }
        };

        for (QChar ch : text)
        {
            if (is_valid(ch))
            {
                validated += ch;
            }
        }

        widgets_.trackName->setText(validated);
    }
}