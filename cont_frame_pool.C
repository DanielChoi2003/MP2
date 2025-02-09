/*
 File: ContFramePool.C

 Author: Daniel Choi
 Date  : 1/28/2025

 */

/*--------------------------------------------------------------------------*/
/*
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates
 *single* frames at a time. Because it does allocate one frame at a time,
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.

 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.

 This can be done in many ways, ranging from extensions to bitmaps to
 free-lists of frames etc.

 IMPLEMENTATION:

 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame,
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool.
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.

 NOTE: If we use this scheme to allocate only single frames, then all
 frames are marked as either FREE or HEAD-OF-SEQUENCE.

 NOTE: In SimpleFramePool we needed only one bit to store the state of
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work,
 revisit the implementation and change it to using two bits. You will get
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.

 DETAILED IMPLEMENTATION:

 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:

 A WORD ABOUT RELEASE_FRAMES():

 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e.,
 not associated with a particular frame pool.

 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete

 */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

ContFramePool *ContFramePool::head = nullptr;
ContFramePool *ContFramePool::tail = nullptr;

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/

/*
 * is the variable _frame_no absolute or relative?
 */

ContFramePool::FrameState ContFramePool::get_state(unsigned long _frame_no)
{

    /*
     * bitmap stores in bytes. This is checking which index _frame_no belongs to.
     * 2 bits per frame. 4 frames per byte.
     */
    unsigned int bitmap_index = _frame_no / 4;
    /*
     * finds the position of _frame_no of 8 bits.
     * Since a frame takes two bits, it must move 2 bits at a time
     */
    unsigned int framePos = (_frame_no % 4) * 2;

    /*
     * Move the bits to the least significant bit
     * Then remove the remaining bits that are not two least significant bits
     */

    unsigned char LSB = (bitmap[bitmap_index] >> framePos) & 0x3;
    // Console::puts("Checking frame state at index ");
    // Console::puti(bitmap_index);
    // Console::puts(" (framePos ");
    // Console::puti(framePos);
    // Console::puts(") Byte value: ");
    // Console::puti(LSB);
    // Console::puts("\n");
    switch (LSB)
    {
    case 0x0:
        return FrameState::Free;
    case 0x1:
        return FrameState::Used;
    case 0x2:
        return FrameState::HoS;
    default:
        Console::puts("ERROR: Invalid frame state detected!\n");

        return FrameState::Error;
    }

    /*
     * 00 = FREE
     * 01 = USED
     * 10 = HEAD-OF-SEQUENCE
     * 11 = Error
     */
}

// 1 bitmap = 1 frame pool?
void ContFramePool::set_state(unsigned long _frame_no, FrameState _state) // absolute?
{
    unsigned int bitmap_index = _frame_no / 4;
    unsigned int framePos = (_frame_no % 4) * 2;
    unsigned char mask = 0x3 << framePos; // 0x11

    switch (_state)
    {
    case FrameState::Used:
        // first clear the bits of the selectd frame position and put 0x01 in there
        bitmap[bitmap_index] = (bitmap[bitmap_index] & ~mask) | (0x1 << framePos);
        break;
    case FrameState::Free:
        bitmap[bitmap_index] &= ~mask;
        break;
    case FrameState::HoS:
        bitmap[bitmap_index] = (bitmap[bitmap_index] & ~mask) | (0x2 << framePos);
        break;
    }
}

// Constructor: Initialize all frames to FREE, except for any frames that you
// need for the management of the frame pool, if any.
// Question: is base frame number an absolute frame number?
// As in not relative to frame pools?
ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no)
{
    // bitmap is stored in a single frame. Ensure that it is able to fit
    // _n_frames will be how many frames bitmap will hold -> 1 frame = 2 bit
    assert(nframes * 2 <= FRAME_SIZE * 8);

    base_frame_no = _base_frame_no;
    nframes = _n_frames;
    nFreeFrames = _n_frames;
    info_frame_no = _info_frame_no;

    // ATTENTION REQUIRED
    // DO NOT TOUCH
    for (int fno = 0; fno < nframes; fno++) // 0 or _base_frame_no?????????????????????
    {
        set_state(fno, FrameState::Free);
    }

    if (info_frame_no == 0) //  if info_frame_no is zero, then use the base memory address
    {
        //  bitmap points to a starting address
        bitmap = (unsigned char *)(FRAME_SIZE * base_frame_no);
    }
    else // if the info_frame_no has a number other than zero, use that number as the address to store info frame
    {
        bitmap = (unsigned char *)(FRAME_SIZE * info_frame_no);
    }

    /*
     * IMPORTANT
     * DO NOT TOUCH
     */
    set_state(info_frame_no, FrameState::HoS);
    nFreeFrames--;

    if (!head)
    {
        head = this;
        tail = this;
    }
    else
    {
        Console::puts("Attaching a frame pool\n");
        tail->next = this;
        this->prev = tail;
        tail = this;
        tail->next = nullptr;
    }

    Console::puts("Frame Pool initialized\n");
}

// get_frames(_n_frames): Traverse the "bitmap" of states and look for a
// sequence of at least _n_frames entries that are FREE. If you find one,
// mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
// ALLOCATED.
unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
    // Any frames left to allocate?
    assert(nFreeFrames > 0);

    unsigned int frame_no = 0;

    // what about HoS? How to know if you are simply getting a new frame or HoS?
    while (get_state(frame_no) == FrameState::Used || get_state(frame_no) == FrameState::HoS)
    {
        // This of course can be optimized!
        frame_no++;
    }

    unsigned int beginning_frame_no = frame_no;

    set_state(frame_no, FrameState::HoS);
    frame_no++;
    nFreeFrames--;

    for (unsigned int i = 0; i < _n_frames - 1; i++)
    {
        if (get_state(frame_no) == FrameState::Used || get_state(frame_no) == FrameState::HoS)
        {
            Console::puts("get_frame(): allocating used or HoS frames\n");
        }
        set_state(frame_no, FrameState::Used);
        frame_no++;
        nFreeFrames--;
    }

    // Console::puts("beginning_frame_no: ");
    // Console::puti(beginning_frame_no);
    // Console::puts("\n");
    // Console::puts("base_frame_no: ");
    // Console::puti(base_frame_no);
    // Console::puts("\n");
    return (beginning_frame_no + base_frame_no);
}

// mark_inaccessible(_base_frame_no, _n_frames): This is no different than
// get_frames, without having to search for the free sequence. You tell the
// allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
// frames after that to mark as ALLOCATED. <- ???????????????????
void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{
    for (int fno = _base_frame_no; fno < _base_frame_no + _n_frames; fno++)
    {
        set_state(fno - this->base_frame_no, FrameState::Used); //  getting the relative index?
    }
}

// release_frames(_first_frame_no): Check whether the first frame is marked as
//  HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
//  Traverse the subsequent frames until you reach one that is FREE or
//  HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.
void ContFramePool::release_frames(unsigned long _first_frame_no) // absolute frame number that marks the first frame to free
{
    // figure which frame pool this frame belongs to.

    // iterate through the linked list and compare the value of base frame to the argument
    ContFramePool *temp = head;
    // Console::puts("temp: ");
    // Console::puti(temp->base_frame_no);
    // Console::puts("\n");
    // Console::puts("_first_frame_no: ");
    // Console::puti(_first_frame_no);
    // Console::puts("\n");
    while (temp)
    {
        if (temp->base_frame_no <= _first_frame_no && _first_frame_no < temp->base_frame_no + temp->nframes) // frame pool found
        {
            // _frame_frame_no: absolute frame number
            // _base_frame_no: starting absolute frame number of the frame pool
            unsigned long frame = _first_frame_no - temp->base_frame_no; // get the relative frame number of the pool
            if (temp->get_state(frame) != FrameState::HoS)               // check if the first frame is HoS
            {
                Console::puts("release_frames(): first frame not a Head-Of-Sequence");
                break;
            }

            Console::puts("First Frame Freed: ");
            Console::puti(frame + temp->base_frame_no);
            Console::puts("\n");

            temp->set_state(frame, FrameState::Free);
            temp->nFreeFrames++;

            frame++;
            // frame < temp->nframes, not <= because frame starts with 0
            // free within the bound of the frame pool
            while (frame < temp->nframes && temp->get_state(frame) == FrameState::Used) // freeing the frames
            {
                // Console::puts("Freeing Frame ");
                // Console::puti(frame);
                // Console::puts("\n");
                temp->set_state(frame, FrameState::Free);

                frame++;
                temp->nFreeFrames++;
            }
            Console::puts("Last Frame Freed: ");
            Console::puti(frame + temp->base_frame_no - 1);
            Console::puts("\n");

            // unlinking the node
            if (temp->nFreeFrames == temp->nframes)
            {
                Console::puts("Entire frame pool is now free, removing from list.\n");

                if (temp->prev)
                    temp->prev->next = temp->next;
                if (temp->next)
                    temp->next->prev = temp->prev;

                if (temp == head)
                    head = temp->next;
                if (temp == tail)
                    tail = temp->prev;

                // delete temp; -> Unnecessay, correct?
            }
            break;
        }

        temp = temp->next;
    }
}

// needed_info_frames(_n_frames): This depends on how many bits you need
//  to store the state of each frame. If you use a char to represent the state
//  of a frame, then you need one info frame for each FRAME_SIZE frames.
unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    /*
     * how many frames do we need?
     * each allocated frames requires 2 bits to store their state
     * so we multiply the number of frames by 2 and divide by frame size
     * and round it up to get the minimum number of info_frames required.
     */
    unsigned long FRAME_SIZE_BITS = FRAME_SIZE * 8;
    unsigned long info_frame_required = (_n_frames * 2 + FRAME_SIZE_BITS - 1) / FRAME_SIZE_BITS;

    return info_frame_required;
}

void ContFramePool::check_freed_frames(unsigned long _first_frame_no, unsigned long _frame_allocated_size)
{
    ContFramePool *temp = head;
    // Console::puts("temp: ");
    // Console::puti(temp->base_frame_no);
    // Console::puts("\n");
    // Console::puts("_first_frame_no: ");
    // Console::puti(_first_frame_no);
    // Console::puts("\n");
    while (temp)
    {
        if (temp->base_frame_no <= _first_frame_no && _first_frame_no < temp->base_frame_no + temp->nframes) // frame pool found
        {
            // _frame_frame_no: absolute frame number
            // _base_frame_no: starting absolute frame number of the frame pool
            unsigned long frame = _first_frame_no - temp->base_frame_no; // get the relative frame number of the pool
            unsigned long end = frame + _frame_allocated_size;
            // frane < temp->nframes, not <= because frame starts with 0
            while (frame < end)
            {
                if (temp->get_state(frame) != FrameState::Free)
                {
                    Console::puts("FRAME NOT FREED PROPERLY\n");
                    Console::puts("Frame number: ");
                    Console::puti(frame + temp->base_frame_no);
                    Console::puts("\n");
                }
                frame++;
            }
            break;
        }

        temp = temp->next;
    }
}
