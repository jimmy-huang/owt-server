/*
 * BufferManager.cpp
 *
 *  Created on: Aug 30, 2014
 *      Author: qzhang8
 */

#include "BufferManager.h"

#include <webrtc/common_video/interface/i420_video_frame.h>

namespace mcu {

DEFINE_LOGGER(BufferManager, "mcu.BufferManager");

BufferManager::BufferManager() {
    for (int i = 0; i < SLOT_SIZE*2; i++) {
    	webrtc::I420VideoFrame* buffer =  new webrtc::I420VideoFrame();
    	buffer->CreateEmptyFrame(640, 480, 640, 640/2, 640/2);
    	freeQ_.push(buffer);
    }

    for (int i = 0; i < SLOT_SIZE; i++) {
    	busyQ_[i] = NULL;
    }

    ELOG_DEBUG("BufferManager constructed")
}

BufferManager::~BufferManager() {
	webrtc::I420VideoFrame* buffer;
	while(freeQ_.pop(buffer)) {
   		delete buffer;
    }
	ELOG_DEBUG("BufferManager destroyed")
}

webrtc::I420VideoFrame* BufferManager::getFreeBuffer() {
	webrtc::I420VideoFrame* buffer;
	if(freeQ_.pop(buffer))
		return buffer;
	else{
		ELOG_DEBUG("freeQ is empty")
		return NULL;
	}
}

void BufferManager::releaseBuffer(webrtc::I420VideoFrame* frame) {
	if (frame != NULL)
		freeQ_.push(frame);
}

webrtc::I420VideoFrame* BufferManager::getBusyBuffer(int slot) {
	assert (slot <= SLOT_SIZE);
	webrtc::I420VideoFrame* busyFrame = BufferManager::exchange((volatile uint64_t*)&(busyQ_[slot]), 0);
	ELOG_DEBUG("getBusyBuffer: busyQ[%d] is 0x%p, busyFrame is 0x%p", slot, busyQ_[slot], busyFrame);
	return busyFrame;
}

webrtc::I420VideoFrame* BufferManager::returnBusyBuffer(
		webrtc::I420VideoFrame* frame, int slot) {
	assert (slot <= SLOT_SIZE);
	webrtc::I420VideoFrame* busyFrame = BufferManager::cmpexchange((volatile uint64_t*)&(busyQ_[slot]), frame, 0);
	ELOG_DEBUG("after returnBusyBuffer: busyQ[%d] is 0x%p, busyFrame is 0x%p", slot, busyQ_[slot], busyFrame);
	return busyFrame;

}

webrtc::I420VideoFrame* BufferManager::postFreeBuffer(webrtc::I420VideoFrame* frame,
		int slot) {
	assert (slot <= SLOT_SIZE);
	ELOG_DEBUG("Posting buffer to slot %d", slot);
	webrtc::I420VideoFrame* busyFrame = BufferManager::exchange((volatile uint64_t*)&(busyQ_[slot]), frame);
	ELOG_DEBUG("after postFreeBuffer: busyQ[%d] is 0x%p,  busyFrame is 0x%p", slot, busyQ_[slot], busyFrame);
	return busyFrame;

}

} /* namespace mcu */
