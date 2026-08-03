import mido

CLOCK=1789772
FRAME_MS=20

def midi_to_freq(n):
    return 440.0*(2**((n-69)/12))

def freq_to_ay_period(f):
    p=int(CLOCK/(16*f))
    if p<1:p=1
    if p>0xFFF:p=0xFFF
    return p

def split_period(v):
    return v&0xFF,(v>>8)&0x0F


def read_track(track,mid,tempo):
    notes=[]
    t=0
    notes_on={}

    for msg in track:

        t+=mido.tick2second(
            msg.time,
            mid.ticks_per_beat,
            tempo
        )

        if msg.type=="note_on" and msg.velocity>0:
            notes_on[msg.note]=t

        elif msg.type=="note_off" or (
            msg.type=="note_on" and msg.velocity==0
        ):
            if msg.note in notes_on:
                start=notes_on.pop(msg.note)

                notes.append({
                    "start":start,
                    "end":t,
                    "period":freq_to_ay_period(
                        midi_to_freq(msg.note)
                    )
                })

    return notes



def get_decay_length(ticks):

    if ticks<=2:return 0
    if ticks<=5:return 0
    if ticks<=7:return 0
    if ticks<=9:return 3
    if ticks<=11:return 8
    return 10



def ease_out(progress):

    v=int(
        15*((1-progress)**2.2)
    )

    if v<0:v=0
    return v



def get_volume(note,time):

    ticks=int(
        (
            note["end"]-
            note["start"]
        )*1000/FRAME_MS
    )


    decay=get_decay_length(ticks)

    if decay==0:
        return 15


    decay_time=(
        decay*
        FRAME_MS/
        1000
    )


    decay_start=(
        note["end"]-
        decay_time
    )


    if time<decay_start:
        return 15


    progress=(
        time-decay_start
    )/decay_time


    if progress>=1:
        return 0


    return ease_out(progress)



def midi_to_pal_bin(midi_file,output_file):

    mid=mido.MidiFile(
        midi_file
    )

    tempo = 50000000

    for msg in mid.tracks[0]:
        if msg.type=="set_tempo":
            tempo=msg.tempo


    channels=[[],[],[]]


    for i in range(
        1,
        min(4,len(mid.tracks))
    ):
        channels[i-1]=read_track(
            mid.tracks[i],
            mid,
            tempo
        )


    duration=0

    for ch in channels:
        for note in ch:
            if note["end"]>duration:
                duration=note["end"]


    frames=int(
        duration*1000/FRAME_MS
    )+1


    print("Ticks:",frames)
    print("Durée:",frames*0.020)



    output=bytearray()


    for frame in range(frames):

        time=(
            frame*
            FRAME_MS/
            1000
        )


        for ch in channels:

            active=None


            for note in ch:

                if (
                    time>=note["start"]
                    and
                    time<note["end"]
                ):
                    active=note
                    break



            if active:

                low,high=split_period(
                    active["period"]
                )

                vol=get_volume(
                    active,
                    time
                )

                output+=bytes([
                    low,
                    high,
                    vol
                ])

            else:

                output+=bytes([
                    0xFF,
                    0xFF,
                    0
                ])


    with open(
        output_file,
        "wb"
    ) as f:
        f.write(output)


    print(
        "Conversion terminée:",
        len(output),
        "octets"
    )



midi_to_pal_bin(
    "SoundDataMidi.mid",
    "soundData.bin"
)