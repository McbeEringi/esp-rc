#ifndef _BMX055_H_
#define _BMX055_H_
class BMX055{
private:
	void _write(uint8_t a,uint8_t x,uint8_t y){Wire.beginTransmission(a);Wire.write(x);Wire.write(y);Wire.endTransmission();}
	void _read6(uint8_t a,uint8_t p,uint8_t *d){for(uint8_t i=0;i<6;i++){
		Wire.beginTransmission(a);Wire.write(p+i);Wire.endTransmission();
		Wire.requestFrom(a,(uint8_t)1);if(Wire.available())d[i]=Wire.read();
	}}
public:
	BMX055(uint8_t a=0x19,uint8_t g=0x69,uint8_t m=0x13){Aa=a;Ga=g;Ma=m;};
	uint8_t Aa,Ga,Ma;
	float Ax=0.,Ay=0.,Az=0.;
	float Gx=0.,Gy=0.,Gz=0.;
	int16_t Mx=0,My=0,Mz=0;
	void init(){
    _write(Aa,0x14,0xb6);delay(2);// A soft reset
    _write(Ga,0x14,0xb6);delay(2);// G soft reset
    _write(Ma,0x4b,0x82);delay(2);// M soft reset
		_write(Aa,0x0f,0x03);// PMU_Range +-2g
		_write(Aa,0x10,0x08);// PMU_BW 7.81 Hz
		_write(Aa,0x11,0x00);// PMU_LPW NORMAL 0.5 ms
		//_write(Ga,0x0f,0x04);// RANGE +-125 deg/s
		_write(Ga,0x10,0x07);// BW 100 Hz 32Hz
		_write(Ga,0x11,0x00);// LPM1 NORMAL 2 ms
    _write(Ma,0x4b,0x01);delay(10);// unsuspend
		_write(Ma,0x4c,0x00);// NORMAL 10Hz
		_write(Ma,0x51,0x04);// RepXY 9
		_write(Ma,0x52,0x0f);// RepZ 15
	};
	void read(){
		uint8_t d[6];
		_read6(Aa,0x02,d);// m/s^2
		Ax=(((int16_t)(d[1]<<8)|d[0])>>4)*.00958;
		Ay=(((int16_t)(d[3]<<8)|d[2])>>4)*.00958;
		Az=(((int16_t)(d[5]<<8)|d[4])>>4)*.00958;
		_read6(Ga,0x02,d);// deg/s
		Gx=((int16_t)(d[1]<<8)|d[0])*.06104;//.00381;
		Gy=((int16_t)(d[3]<<8)|d[2])*.06104;//.00381;
		Gz=((int16_t)(d[5]<<8)|d[4])*.06104;//.00381;
		_read6(Ma,0x42,d);
		Mx=((int16_t)(d[1]<<8)|d[0])>>3;
		My=((int16_t)(d[3]<<8)|d[2])>>3;
		Mz=((int16_t)(d[5]<<8)|d[4])>>1;
	};
};
#endif