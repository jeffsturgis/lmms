/*
 * Lv2Instrument.h - implementation of LV2 instrument
 *
 * Copyright (c) 2018-2023 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#ifndef LV2_INSTRUMENT_H
#define LV2_INSTRUMENT_H

#include <QString>

#include "Instrument.h"
#include "InstrumentView.h"
#include "Note.h"
#include "Lv2ControlBase.h"
#include "Lv2ViewBase.h"

// whether to use MIDI vs playHandle
// currently only MIDI works
#define LV2_INSTRUMENT_USE_MIDI

namespace lmms
{

namespace gui
{

class Lv2InsView;

}

class Lv2Instrument : public Instrument, public Lv2ControlBase
{
	Q_OBJECT
signals:
	void modelChanged();
public:
	/*
		initialization
	*/
	Lv2Instrument(InstrumentTrack *instrumentTrackArg,
		 Descriptor::SubPluginFeatures::Key* key);
	~Lv2Instrument() override;
	void reload();
	void onSampleRateChanged();
	//! Must be checked after ctor or reload
	bool isValid() const;

	/*
		load/save
	*/
	void saveSettings(QDomDocument &doc, QDomElement &that) override;
	void loadSettings(const QDomElement &that) override;
	void loadFile(const QString &file) override;

	/*
		realtime funcs
	*/
	bool hasNoteInput() const override { return Lv2ControlBase::hasNoteInput(); }
#ifdef LV2_INSTRUMENT_USE_MIDI
	bool handleMidiEvent(const MidiEvent &event,
		const TimePos &time = TimePos(), f_cnt_t offset = 0) override;
#else
	void playNote(NotePlayHandle *nph, sampleFrame *) override;
#endif
	void play(sampleFrame *buf) override;

	/*
		misc
	*/
	Flags flags() const override
	{
#ifdef LV2_INSTRUMENT_USE_MIDI
		return Flag::IsSingleStreamed | Flag::IsMidiBased;
#else
		return Flag::IsSingleStreamed;
#endif
	}
	gui::PluginView* instantiateView(QWidget *parent) override;

private slots:
	void updatePitchRange();

private:
	QString nodeName() const override;

#ifdef LV2_INSTRUMENT_USE_MIDI
	std::array<int, NumKeys> m_runningNotes = {};
#endif
	void clearRunningNotes();

	friend class gui::Lv2InsView;
};


namespace gui
{


class Lv2InsView : public InstrumentView, public Lv2ViewBase
{
Q_OBJECT
public:
	Lv2InsView(Lv2Instrument *_instrument, QWidget *_parent);

protected:
	void dragEnterEvent(QDragEnterEvent *_dee) override;
	void dropEvent(QDropEvent *_de) override;
	void hideEvent(QHideEvent* event) override;

private:
	void modelChanged() override;
};


} // namespace gui


} // namespace lmms

#endif // LV2_INSTRUMENT_H
