/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore BVBA and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "mixerchannel.h"
#include <algorithm>
#include <cstring>
#include "log.h"

using namespace mu::audio;

MixerChannel::MixerChannel()
{
}

unsigned int MixerChannel::audioChannelsCount() const
{
    IF_ASSERT_FAILED(m_source) {
        return 0;
    }
    return m_source->audioChannelsCount();
}

void MixerChannel::checkStreams()
{
    if (audioChannelsCount() != m_level.size()) {
        updateBalanceLevelMaps();
    }
}

void MixerChannel::process(float* buffer, unsigned int sampleCount)
{
    IF_ASSERT_FAILED(m_source) {
        return;
    }
    m_source->process(buffer, sampleCount);

    for (std::pair<const unsigned int, IAudioProcessorPtr>& p : m_processorList) {
        if (p.second->active()) {
            p.second->process(buffer, buffer, sampleCount);
        }
    }
}

void MixerChannel::setSampleRate(unsigned int sampleRate)
{
    AbstractAudioSource::setSampleRate(sampleRate);
    IF_ASSERT_FAILED(m_source) {
        return;
    }
    m_source->setSampleRate(sampleRate);
    for (std::pair<const unsigned int, IAudioProcessorPtr>& p : m_processorList) {
        p.second->setSampleRate(sampleRate);
    }
}

void MixerChannel::setSource(std::shared_ptr<IAudioSource> source)
{
    m_source = source;
    updateBalanceLevelMaps();
    m_source->audioChannelsCountChanged().onReceive(this, [this](unsigned int) {
        updateBalanceLevelMaps();
    });
    setActive(true);
}

bool MixerChannel::active() const
{
    return m_active;
}

void MixerChannel::setActive(bool active)
{
    m_active = active;
}

float MixerChannel::level(unsigned int streamId) const
{
    IF_ASSERT_FAILED(m_level.find(streamId) != m_level.end()) {
        return 0.f;
    }
    return m_level.at(streamId);
}

void MixerChannel::setLevel(float level)
{
    for (auto& stream : m_level) {
        m_level[stream.first] = level;
    }
}

void MixerChannel::setLevel(unsigned int streamId, float level)
{
    m_level[streamId] = level;
}

std::complex<float> MixerChannel::balance(unsigned int streamId) const
{
    IF_ASSERT_FAILED(m_balance.find(streamId) != m_balance.end()) {
        return 0.f;
    }
    return m_balance.at(streamId);
}

void MixerChannel::setBalance(std::complex<float> value)
{
    for (auto& stream : m_balance) {
        m_balance[stream.first] = value;
    }
}

void MixerChannel::setBalance(unsigned int streamId, std::complex<float> value)
{
    m_balance[streamId] = value;
}

IAudioProcessorPtr MixerChannel::processor(unsigned int number) const
{
    IF_ASSERT_FAILED(m_processorList.find(number) != m_processorList.end()) {
        return nullptr;
    }
    return m_processorList.at(number);
}

void MixerChannel::setProcessor(unsigned int number, IAudioProcessorPtr proc)
{
    IF_ASSERT_FAILED(proc->streamCount() == audioChannelsCount()) {
        LOGE() << "Processor's stream count not equal to the channel";
        return;
    }
    proc->setSampleRate(m_sampleRate);
    m_processorList[number] = proc;
}

void MixerChannel::updateBalanceLevelMaps()
{
    for (unsigned int c = 0; c < m_source->audioChannelsCount(); ++c) {
        m_level[c] = 1.f;

        if (m_source->audioChannelsCount() > 1) {
            m_balance[c] = 2.f * c / (m_source->audioChannelsCount() - 1) - 1.f;
        } else {
            m_balance[c] = 0.f;
        }
    }
}
