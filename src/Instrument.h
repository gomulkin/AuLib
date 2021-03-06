////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2016-7 V Lazzarini
// Note.h: note and instrument modelling classes
//
// This software is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
// version 3.0 of the License, or (at your option) any later version.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef __INSTRUMENT_H__
#define __INSTRUMENT_H__

#include "AudioBase.h"
#include "Note.h"
#include <vector>

namespace AuLib {

/** Instrument template class: \n
    Creates an instrument with n voices when instantiated with a
    Note-derived class with optional extra parameters.
*/
template <typename T, typename... Targs> class Instrument : public AudioBase {

  std::vector<T> m_voices;
  std::vector<double> m_msgdata;
  uint32_t m_msg;
  int32_t m_chn;
  uint64_t m_stamp;

  /** specialise this to handle poliphony and
      dispatched messages. The base implementation uses
      last-note priority, parsing MIDI note on and note off
      messages only.
  */
  virtual void msg_handler();

  /** basic processing method, can be specialised.
   */
  virtual const Instrument &dsp() {
    set(0.);
    for (auto &note : m_voices) {
      note.process();
      *this += note;
    }
    return *this;
  }

public:
  /** Construct an instrument with nvoices polyphony
   */
  Instrument(uint32_t nvoices, int32_t chn, Targs... args)
      : m_voices(nvoices, T(chn, args...)), m_msgdata(256), m_msg(0),
        m_chn(chn), m_stamp(0) {}

  /** Dispatch a channel message using a MIDI-like format. \n
     msg - message type \n
     chn - channel \n
     data1 - message data 1 \n
     data2 - message data 2 \n
     stamp - time stamp \n\n
     Note that message types are not constrained to MIDI ones; the
     Note class can be specialised to handle any pre-defined messages.
   */
  void dispatch(uint32_t msg, uint32_t chn, double data1, double data2,
                uint64_t stamp) {
    m_stamp = stamp;
    m_chn = chn;
    m_msg = msg;
    m_msgdata.assign({data1, data2});
    msg_handler();
  }

  /** Dispatch a channel message of unspecified length \n
     msg - type \n
     chn - chn \n
     data - message data \n
     tstamp - time stamp \n
   */
  void dispatch(uint32_t msg, uint32_t chn, const std::vector<double> &data,
                uint64_t stamp) {
    m_stamp = stamp;
    m_chn = chn;
    m_msg = msg;
    m_msgdata = data;
    msg_handler();
  }

  /** processing interface
   */
  virtual const Instrument &process() { return dsp(); }
};

/** @private
    msg_handler implementation.
*/
template <typename T, typename... Targs>
void Instrument<T, Targs...>::msg_handler() {

  if (m_msg == midi::note_on && m_msgdata[1] == 0)
    m_msg = midi::note_off;
  if (m_msg == midi::note_on) {
    for (auto &note : m_voices) {
      if (!note.is_on()) {
        note.note_on(m_chn, m_msgdata[0], m_msgdata[1], m_stamp);
        return;
      }
    }
    auto ostamp = m_voices[0].time_stamp();
    auto oldest = &m_voices[0];
    for (auto &note : m_voices) {
      if (note.time_stamp() < ostamp) {
        oldest = &note;
        ostamp = note.time_stamp();
      }
    }
    oldest->note_off();
    oldest->note_on(m_chn, m_msgdata[0], m_msgdata[1], m_stamp);
  } else if (m_msg == midi::note_off) {
    for (auto &note : m_voices) {
      note.note_off(m_chn, m_msgdata[0], m_msgdata[1]);
    }
  } else {
    for (auto &note : m_voices) {
      note.ctrl_msg(m_chn, m_msg, m_msgdata, m_stamp);
    }
  }
}

/*! \class Instrument Instrument.h AuLib/Instrument.h
 */
}
#endif
