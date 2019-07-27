#include <Adafruit_NeoPixel.h>

typedef unsigned char uchar;  // unsigned char型をuchar型として定義

//スイッチ接続ピンとスイッチの極性を設定
#define SW1PIN 6
#define SW2PIN 7
#define SW3PIN 8
#define SW4PIN 9
#define SW_ACTIVE HIGH

//NeoPixel(フルカラーLEDモジュール)の接続PINを設定
#define NEOPIXEL_PIN        2
#define NEOPIXEL_PIN_B        3
#define NUMPIXELS 8   //それぞれの縦列（直列）接続数

//Adafruit NeoPixelインスタンスの生成　　今回2つ使用
Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels_B(NUMPIXELS, NEOPIXEL_PIN_B, NEO_GRB + NEO_KHZ800);

#define DELAYVAL 20

void setup() {
	//スイッチのつながったピンすべてを入力に設定
  pinMode(SW1PIN, INPUT);
  pinMode(SW2PIN, INPUT);
  pinMode(SW3PIN, INPUT);
  pinMode(SW4PIN, INPUT);
  
  pixels.begin();  //NeoPixelインスタンス初期化
  pixels.clear();  //NeoPixel表示内容消去（すべて0に）
  pixels.show();   //NeoPixel表示内容更新(ここで信号が送出される　このプログラムが走る前になにか表示されてるかもしれないので)
  pixels_B.begin(); //以下同様
  pixels_B.clear();
  pixels_B.show();
}

//スイッチのチャタリング除去プログラム タイマー(uchar 参照渡し)、digitalReadで使うsw番号、スイッチ極性、タイマーしきい値低い側・高い側、カウンタ上限、前回の出力(0かそうでないか)
uchar checksw_nochatter(uchar& timer, uchar swnum, const uchar sw_polar, uchar threshold_L, uchar threshold_H, uchar cntrlimit, uchar prev_ans){

  if(digitalRead(swnum)==sw_polar){
    if(timer < cntrlimit) timer++;
  }
  else{
    if(timer > 0) timer--;
  }
  
  if(prev_ans != 0){  //前が0でなかった時：L側のスレッショルド適用
    if(timer < threshold_L){
      return 0;
    }
    else{
      return 1;
    }
  }
  else{  //前が0：H側のスレッショルド適用
    if(timer < threshold_H){
      return 0;
    }
    else{
      return 1;
    }
  }
}

uchar receiveSW(void){
  static uchar timer_sw1;
  static uchar timer_sw2;
  static uchar timer_sw3;
  static uchar prev_flags;
  uchar flags = 0;

  if( checksw_nochatter(timer_sw1, SW1PIN, SW_ACTIVE, 1, 3, 5, prev_flags & 0x01) ){
    flags |= 0x01;
    prev_flags |= 0x01;
  }
  else{
    flags &= 0xFE;
    prev_flags &= 0xFE;
  }

  if( checksw_nochatter(timer_sw2, SW2PIN, SW_ACTIVE, 1, 3, 5, prev_flags & 0x02) ){
    flags |= 0x02;
    prev_flags |= 0x02;
  }
  else{
    flags &= 0xFD;
    prev_flags &= 0xFD;
  }

  if( checksw_nochatter(timer_sw3, SW3PIN, SW_ACTIVE, 1, 3, 5, prev_flags & 0x04) ){
    flags |= 0x04;
    prev_flags |= 0x04;
  }
  else{
    flags &= 0xFB;
    prev_flags &= 0xFB;
  } 
  return flags;
}

uchar LEDcolor=0;
uchar swstat=0;
uchar swstat_plusdiff=0;
uchar swstat_minusdiff=0;

void getsw(){
  uchar swdiff_temp = swstat;
  swstat = receiveSW();
  swdiff_temp = swdiff_temp ^ swstat; // XOR : 変化点のみ1
  swstat_plusdiff = swstat & swdiff_temp; // 変化点のうち1になったもの
  swstat_minusdiff = swdiff_temp & (~swstat_plusdiff);
}




/*
SW3でwave / 個別指定トグル
SW1:wave速度切替/色切り替え
SW2:wave切り替え/決定

wave:
0,1,2,3,4,5,6 : 青シアン緑イエロー赤マゼンタ白
7,8,9, : 位相を変えて
10,11,12, :色バランス変える 赤、緑、青強め
*/
uchar main_phase = 0;
uchar drivemode=0; // 0:個別 1:wave

// テーブルで変える
uchar r_phasediff = 0;
uchar g_phasediff = 90;
uchar b_phasediff = 180;
uchar r_enable=1,g_enable=1,b_enable=1;
uchar r_shift=1, g_shift=1, b_shift=1;

#define WAVE_MODE_SIZE 14
const uchar wavetable_r_phasediff[WAVE_MODE_SIZE] = {0,0,0, 0,0,0,0, 0,0,0, 0,0,0, 0}; //赤 位相
const uchar wavetable_g_phasediff[WAVE_MODE_SIZE] = {0,0,0, 0,0,0,0, 30,60,90, 90,90,90, 120}; // 緑　位相
const uchar wavetable_b_phasediff[WAVE_MODE_SIZE] = {0,0,0, 0,0,0,0, 60,120,180, 180,180,180, 170}; // 青　位相
const uchar wavetable_r_enable[WAVE_MODE_SIZE] = {0,0,0, 1,1,1,1, 1,1,1, 1,1,1, 1}; //赤　消灯0 / 点灯1
const uchar wavetable_g_enable[WAVE_MODE_SIZE] = {0,1,1, 0,0,1,1, 1,1,1, 1,1,1, 1}; //緑　消灯0 / 点灯1　
const uchar wavetable_b_enable[WAVE_MODE_SIZE] = {1,0,1, 0,1,0,1, 1,1,1, 1,1,1, 1}; //青 　消灯0 / 点灯1
const uchar wavetable_r_shift[WAVE_MODE_SIZE] =  {1,1,1, 1,1,1,1, 1,1,1, 3,3,1, 1}; //赤 明るさシフト量 大きいほど暗くなる
const uchar wavetable_g_shift[WAVE_MODE_SIZE] =  {1,1,1, 1,1,1,1, 1,1,1, 3,1,3, 3}; //緑　明るさシフト量
const uchar wavetable_b_shift[WAVE_MODE_SIZE] =  {1,1,1, 1,1,1,1, 1,1,1, 1,3,3, 2}; //青　明るさシフト量

//スイッチで変える
uchar phase_posspeed=5;
uchar wave_mode = 0;

void loop() {
  delay(DELAYVAL);  //
  
  getsw();
  if(swstat_plusdiff & 0x04){
    drivemode = (drivemode)? 0 : 1;
    Serial.print("set drivemode: ");
    Serial.println(drivemode);
  }
  if(drivemode==0){

    if(swstat_plusdiff & 0x01){
      pixels.setPixelColor(0, pixels.Color(((LEDcolor&0x04)? 100:0), ((LEDcolor&0x02)? 100:0), ((LEDcolor&0x01)? 100:0)));
      LEDcolor++;
      if(LEDcolor > 7) LEDcolor = 0;
    }
    if(swstat_plusdiff & 0x02){
      for(int i=NUMPIXELS-1;i>=1;i--){
        pixels_B.setPixelColor(i,pixels_B.getPixelColor(i-1));
      }
      pixels_B.setPixelColor(0,pixels.getPixelColor(NUMPIXELS-1));
      for(int i=NUMPIXELS-1;i>=1;i--){
        pixels.setPixelColor(i,pixels.getPixelColor(i-1));
      }
    }
  
    if(swstat_minusdiff){
      pixels.show(); 
      pixels_B.show(); 
    }
    return;
  }
  else{
    
    if(swstat_plusdiff & 0x01){
      phase_posspeed += 5;
      if(phase_posspeed > 80) phase_posspeed = 0;
      Serial.print("set phase_posspeed: ");
      Serial.println(phase_posspeed);
    }
    if(swstat_plusdiff & 0x02){
      wave_mode++;
      if(wave_mode >= WAVE_MODE_SIZE){
        wave_mode = 0;
      }
      Serial.print("set wave_mode: ");
      Serial.println(wave_mode);

      r_phasediff=wavetable_r_phasediff[wave_mode];
      g_phasediff=wavetable_g_phasediff[wave_mode];
      b_phasediff=wavetable_b_phasediff[wave_mode];
      r_enable=wavetable_r_enable[wave_mode];
      g_enable=wavetable_g_enable[wave_mode];
      b_enable=wavetable_b_enable[wave_mode];
      r_shift=wavetable_r_shift[wave_mode];
      g_shift=wavetable_g_shift[wave_mode];
      b_shift=wavetable_b_shift[wave_mode];
    }
      
    uchar phase_posdiff=0;
    for(int i=0; i<NUMPIXELS; i++) {
      uchar r_temp=0, g_temp=0, b_temp=0;
      if(r_enable){
        r_temp = Adafruit_NeoPixel::sine8((main_phase + r_phasediff + phase_posdiff) & 0xFF);
      }
      if(g_enable){
        g_temp = Adafruit_NeoPixel::sine8((main_phase + g_phasediff + phase_posdiff) & 0xFF);
      }
      if(b_enable){
        b_temp = Adafruit_NeoPixel::sine8((main_phase + b_phasediff + phase_posdiff) & 0xFF);
      }
      
      pixels.setPixelColor(i, pixels.Color(r_temp >> r_shift, g_temp >> g_shift, b_temp >> b_shift));
  
      phase_posdiff += phase_posspeed;
    }
    phase_posdiff += (phase_posspeed << 2);
    for(int i=0; i<NUMPIXELS; i++) {
      uchar r_temp=0, g_temp=0, b_temp=0;
      if(r_enable){
        r_temp = Adafruit_NeoPixel::sine8((main_phase + r_phasediff + phase_posdiff) & 0xFF);
      }
      if(g_enable){
        g_temp = Adafruit_NeoPixel::sine8((main_phase + g_phasediff + phase_posdiff) & 0xFF);
      }
      if(b_enable){
        b_temp = Adafruit_NeoPixel::sine8((main_phase + b_phasediff + phase_posdiff) & 0xFF);
      }
      
      pixels_B.setPixelColor(i, pixels.Color(r_temp >> r_shift, g_temp >> g_shift, b_temp >> b_shift));
  
      phase_posdiff += phase_posspeed;
    }
    pixels.show(); 
    pixels_B.show(); 
    main_phase++;

  }
}
