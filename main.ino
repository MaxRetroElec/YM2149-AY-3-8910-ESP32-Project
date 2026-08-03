#include <Arduino.h>
#include <LittleFS.h>

const int DATA_PINS[8] = {
    19, 21, 14, 27,
    26, 25, 33, 32
};

const int BC1  = 15;
const int RST  = 5;
const int BDIR = 18;

const int CLOCK_PIN = 22;

hw_timer_t *timer = NULL;

volatile bool tick50Hz = false;


File soundFile;


void setDataBus(uint8_t value)
{
    for(int i = 0; i < 8; i++)
    {
        digitalWrite(
            DATA_PINS[i],
            (value >> i) & 1
        );
    }
}



void ymWrite(uint8_t reg, uint8_t value)
{
    setDataBus(reg);

    digitalWrite(BC1, HIGH);
    digitalWrite(BDIR, HIGH);

    delayMicroseconds(2);

    digitalWrite(BDIR, LOW);
    digitalWrite(BC1, LOW);


    setDataBus(value);


    digitalWrite(BC1, LOW);
    digitalWrite(BDIR, HIGH);

    delayMicroseconds(2);

    digitalWrite(BDIR, LOW);
}



void ymReset()
{
    digitalWrite(RST, LOW);
    delay(10);

    digitalWrite(RST, HIGH);
    delay(10);
}

void startClock()
{
    ledcAttach(
        CLOCK_PIN,
        1789772,
        1
    );

    ledcWrite(
        CLOCK_PIN,
        1
    );
}

void IRAM_ATTR onTick()
{
    tick50Hz = true;
}


void executeTick()
{

    if(soundFile.available() < 9)
    {
        ymWrite(8,0);
        ymWrite(9,0);
        ymWrite(10,0);

        return;
    }


    uint8_t aLow  = soundFile.read();
    uint8_t aHigh = soundFile.read();
    uint8_t aVol  = soundFile.read();


    uint8_t bLow  = soundFile.read();
    uint8_t bHigh = soundFile.read();
    uint8_t bVol  = soundFile.read();


    uint8_t cLow  = soundFile.read();
    uint8_t cHigh = soundFile.read();
    uint8_t cVol  = soundFile.read();


    if(!(aLow == 0xFF && aHigh == 0xFF))
    {
        ymWrite(0, aLow);
        ymWrite(1, aHigh & 0x0F);

        if(aLow==0xFF || aHigh>0x0F)
        {
            Serial.println("ERREUR DATA A");
        }

        ymWrite(8, aVol & 0x0F);
    }
    else
    {
        ymWrite(0,0);
        ymWrite(1,0);
        ymWrite(8,0);
    }


    if(!(bLow == 0xFF && bHigh == 0xFF))
    {
        ymWrite(2, bLow);
        ymWrite(3, bHigh & 0x0F);

        if(bLow==0xFF || bHigh>0x0F)
        {
            Serial.println("ERREUR DATA B");
        }

        ymWrite(9, bVol & 0x0F);
    }
    else
    {
        ymWrite(2,0);
        ymWrite(3,0);
        ymWrite(9,0);
    }


    if(!(cLow == 0xFF && cHigh == 0xFF))
    {
        ymWrite(4, cLow);
        ymWrite(5, cHigh & 0x0F);

        if(cLow==0xFF || cHigh>0x0F)
        {
            Serial.println("ERREUR DATA C");
        }

        ymWrite(10, cVol & 0x0F);
    }
    else
    {
        ymWrite(4,0);
        ymWrite(5,0);
        ymWrite(10,0);
    }

}


void setup()
{

    Serial.begin(115200);

    for(int i = 0; i < 8; i++)
    {
        pinMode(
            DATA_PINS[i],
            OUTPUT
        );

        digitalWrite(
            DATA_PINS[i],
            LOW
        );
    }



    pinMode(BC1,OUTPUT);
    pinMode(BDIR,OUTPUT);
    pinMode(RST,OUTPUT);
    pinMode(23,OUTPUT);



    digitalWrite(BC1,LOW);
    digitalWrite(BDIR,LOW);
    digitalWrite(23,HIGH);


    if(!LittleFS.begin(true))
    {
        Serial.println("LittleFS KO");

        while(true);
    }



    soundFile = LittleFS.open(
        "/soundData.bin",
        "r"
    );


    if(!soundFile)
    {
        Serial.println(
            "Erreur ouverture fichier"
        );

        while(true);
    }


    startClock();

    ymReset();

    ymWrite(7, 0b00111000);

    ymWrite(8,15);
    ymWrite(9,15);
    ymWrite(10,15);

    ymWrite(0,0x75);
    ymWrite(1,0x04);

    delay(1000);

    ymWrite(2,0x75);
    ymWrite(3,0x04);

    delay(1000);

    ymWrite(0,0x75);
    ymWrite(1,0x04);

    delay(1000);

    timer = timerBegin(
        1000000
    );


    timerAttachInterrupt(
        timer,
        &onTick
    );


    timerAlarm(
        timer,
        20000,
        true,
        0
    );


    Serial.println(
        "Lecteur YM2149 PAL 50Hz"
    );

}


void loop()
{

    if(tick50Hz)
    {
        tick50Hz = false;

        executeTick();
    }

}