// ADS1299_Registers.h
// Mapa de registros y campos del ADS1299-x con helpers para configuración segura.
// Referencias principales: datasheet ADS1299, secciones 9.5 Programming y 9.6 Register Maps.

#pragma once
#include <stdint.h>

// =========================
// SPI (9.5) — comandos/opcodes
// =========================
enum : uint8_t {
  ADS_CMD_WAKEUP  = 0x02, // 9.5.3.2
  ADS_CMD_STANDBY = 0x04, // 9.5.3.3
  ADS_CMD_RESET   = 0x06, // 9.5.3.4
  ADS_CMD_START   = 0x08, // 9.5.3.5
  ADS_CMD_STOP    = 0x0A, // 9.5.3.6
  ADS_CMD_RDATAC  = 0x10, // 9.5.3.7
  ADS_CMD_SDATAC  = 0x11, // 9.5.3.8
  ADS_CMD_RDATA   = 0x12, // 9.5.3.9
  ADS_CMD_RREG    = 0x20, // 9.5.3.10 (OR con addr)
  ADS_CMD_WREG    = 0x40, // 9.5.3.11 (OR con addr)
  ADS_CMD_NOP     = 0x00
};

// =========================
// Direcciones de registros (9.6)
// =========================
enum : uint8_t {
  ADS_REG_ID         = 0x00, // 9.6.1.1
  ADS_REG_CONFIG1    = 0x01, // 9.6.1.2
  ADS_REG_CONFIG2    = 0x02, // 9.6.1.3
  ADS_REG_CONFIG3    = 0x03, // 9.6.1.4
  ADS_REG_LOFF       = 0x04, // 9.6.1.5
  ADS_REG_CH1SET     = 0x05, // 9.6.1.6 (CH2..CH8 = +1..+7)
  ADS_REG_CH2SET     = 0x06,
  ADS_REG_CH3SET     = 0x07,
  ADS_REG_CH4SET     = 0x08,
  ADS_REG_CH5SET     = 0x09,
  ADS_REG_CH6SET     = 0x0A,
  ADS_REG_CH7SET     = 0x0B,
  ADS_REG_CH8SET     = 0x0C,
  ADS_REG_BIAS_SENSP = 0x0D, // 9.6.1.7
  ADS_REG_BIAS_SENSN = 0x0E, // 9.6.1.8
  ADS_REG_LOFF_SENSP = 0x0F, // 9.6.1.9
  ADS_REG_LOFF_SENSN = 0x10, // 9.6.1.10
  ADS_REG_LOFF_FLIP  = 0x11, // 9.6.1.11
  ADS_REG_LOFF_STATP = 0x12, // 9.6.1.12 (R)
  ADS_REG_LOFF_STATN = 0x13, // 9.6.1.13 (R)
  ADS_REG_GPIO       = 0x14, // 9.6.1.14
  ADS_REG_MISC1      = 0x15, // 9.6.1.15
  ADS_REG_MISC2      = 0x16, // 9.6.1.16 (reservado)
  ADS_REG_CONFIG4    = 0x17  // 9.6.1.17
};

// =========================
// ID (0x00) — 9.6.1.1
// =========================
// Bits [7:5] REV_ID, [4]=1 fijo, [3:2] DEV_ID, [1:0] NU_CH
// NU_CH: 00=ADS1299-4, 01=ADS1299-6, 10=ADS1299.
#define ADS_ID_REV_ID_MASK    0xE0
#define ADS_ID_DEV_ID_MASK    0x0C
#define ADS_ID_NU_CH_MASK     0x03
#define ADS_ID_DEV_IS_1299(id) (((id) & ADS_ID_DEV_ID_MASK) == 0x0C)

// =========================
// CONFIG1 (0x01) — 9.6.1.2
// =========================
// [7]=1 fijo, [6]=DAISY_EN, [5]=CLK_EN, [4:3]=10 fijo, [2:0]=DR
// Nota importante del datasheet:
//   DAISY_EN = 0 -> Daisy-chain mode
//   DAISY_EN = 1 -> Multiple readback mode
// Por compatibilidad mantenemos el nombre del bit del datasheet, aunque su
// semántica no es "enable daisy" en sentido intuitivo.
#define ADS_CFG1_DAISY_EN            0x40
#define ADS_CFG1_MULTIPLE_READBACK   ADS_CFG1_DAISY_EN
#define ADS_CFG1_CLK_EN              0x20

// DR data rate bits [2:0]. Con fCLK=2.048 MHz, DR=110 -> 250 SPS.
enum : uint8_t {
  ADS_DR_16k = 0b000,
  ADS_DR_8k  = 0b001,
  ADS_DR_4k  = 0b010,
  ADS_DR_2k  = 0b011,
  ADS_DR_1k  = 0b100,
  ADS_DR_500 = 0b101,
  ADS_DR_250 = 0b110,
  // 111 reservado
};

#define ADS_CFG1_RESERVED_FIXED 0x90 // bit7=1 y bits[4:3]=10.
#define ADS_CFG1_MAKE(multipleReadback, clk_en, dr) \
  (uint8_t)(ADS_CFG1_RESERVED_FIXED | ((multipleReadback) ? ADS_CFG1_MULTIPLE_READBACK : 0) | ((clk_en) ? ADS_CFG1_CLK_EN : 0) | ((dr) & 0x07))

// CONFIG1 histórico/probado: 0x96 = 250 SPS, CLK_OUT off, DAISY_EN=0.
static constexpr uint8_t ADS_CFG1_250SPS =
  ADS_CFG1_MAKE(false /*DAISY_EN=0: daisy-chain mode*/, false /*clk_out*/, ADS_DR_250);

// =========================
// CONFIG2 (0x02) — 9.6.1.3 (test signal)
// =========================
// [7:6]=11 fijo, [4]=INT_CAL, [2]=CAL_AMP, [1:0]=CAL_FREQ
#define ADS_CFG2_INT_CAL     0x10
#define ADS_CFG2_CAL_AMP_1X  0x00 // 1 × -(VREFP - VREFN) / 2400
#define ADS_CFG2_CAL_AMP_2X  0x04 // 2 × -(VREFP - VREFN) / 2400

enum : uint8_t {
  ADS_CALF_CLK_2_21 = 0b00, // pulsed at fCLK / 2^21
  ADS_CALF_CLK_2_20 = 0b01, // pulsed at fCLK / 2^20
  ADS_CALF_RSVD     = 0b10, // do not use
  ADS_CALF_DC       = 0b11  // at dc
};

#define ADS_CFG2_MAKE(intcal, amp2x, freq2b) \
  (uint8_t)(0xC0 | ((intcal) ? ADS_CFG2_INT_CAL : 0) | ((amp2x) ? ADS_CFG2_CAL_AMP_2X : ADS_CFG2_CAL_AMP_1X) | ((freq2b) & 0x03))

// CONFIG2: test interno apagado.
static constexpr uint8_t ADS_CFG2_TEST_OFF =
  ADS_CFG2_MAKE(false, false, ADS_CALF_CLK_2_21);

// =========================
// CONFIG3 (0x03) — 9.6.1.4 (referencia y BIAS)
// =========================
// [7]=PD_REFBUF, [4]=BIAS_MEAS, [3]=BIASREF_INT,
// [2]=PD_BIAS, [1]=BIAS_LOFF_SENS, [0]=BIAS_STAT (R)
#define ADS_CFG3_PD_REFBUF      0x80
#define ADS_CFG3_BIAS_MEAS      0x10
#define ADS_CFG3_BIASREF_INT    0x08
#define ADS_CFG3_PD_BIAS        0x04
#define ADS_CFG3_BIAS_LOFF_SENS 0x02

#define ADS_CFG3_RESERVED_FIXED 0x60 // bits[6:5]=11.
#define ADS_CFG3_MAKE(useIntRef, biasMeas, biasRefInt, biasOn, biasLoffSens) \
  (uint8_t)(ADS_CFG3_RESERVED_FIXED | ((useIntRef) ? ADS_CFG3_PD_REFBUF : 0) | ((biasMeas) ? ADS_CFG3_BIAS_MEAS : 0) | ((biasRefInt) ? ADS_CFG3_BIASREF_INT : 0) | ((biasOn) ? ADS_CFG3_PD_BIAS : 0) | ((biasLoffSens) ? ADS_CFG3_BIAS_LOFF_SENS : 0))

// CONFIG3: referencia interna ON, BIAS drive OFF.
static constexpr uint8_t ADS_CFG3_INTREF_NO_BIAS =
  ADS_CFG3_MAKE(true /*refbuf*/, false /*bias_meas*/, true /*biasref_int*/, false /*bias_on*/, false /*bias_loff_sens*/);

// =========================
// LOFF (0x04) — 9.6.1.5 (lead-off control)
// =========================
// [7:5]=COMP_TH, [3:2]=ILEAD_OFF, [1:0]=FLEAD_OFF
// COMP_TH según datasheet: porcentaje positivo / negativo complementario.
#define ADS_LOFF_COMPTH_95    (0b000 << 5)
#define ADS_LOFF_COMPTH_92_5  (0b001 << 5)
#define ADS_LOFF_COMPTH_90    (0b010 << 5)
#define ADS_LOFF_COMPTH_87_5  (0b011 << 5)
#define ADS_LOFF_COMPTH_85    (0b100 << 5)
#define ADS_LOFF_COMPTH_80    (0b101 << 5)
#define ADS_LOFF_COMPTH_75    (0b110 << 5)
#define ADS_LOFF_COMPTH_70    (0b111 << 5)

#define ADS_LOFF_I_6nA        (0b00 << 2)
#define ADS_LOFF_I_24nA       (0b01 << 2)
#define ADS_LOFF_I_6uA        (0b10 << 2)
#define ADS_LOFF_I_24uA       (0b11 << 2)

#define ADS_LOFF_F_DC         0b00
#define ADS_LOFF_F_7_8HZ      0b01
#define ADS_LOFF_F_31_2HZ     0b10
#define ADS_LOFF_F_FDR_4      0b11

#define ADS_LOFF_MAKE(comp, ilead, flead) \
  (uint8_t)((comp) | (ilead) | ((flead) & 0x03))

// Constante legacy: reproduce el byte histórico probado (0x66). En versiones
// anteriores se llamaba "80pct", pero los bits 011 corresponden a 87.5%.
static constexpr uint8_t ADS_LOFF_AC_24NA_31HZ_87_5PCT_LEGACY =
  ADS_LOFF_MAKE(ADS_LOFF_COMPTH_87_5, ADS_LOFF_I_24nA, ADS_LOFF_F_31_2HZ);

// Constante corregida: 80% real según tabla COMP_TH del datasheet (byte 0xA6).
static constexpr uint8_t ADS_LOFF_AC_24NA_31HZ_80PCT =
  ADS_LOFF_MAKE(ADS_LOFF_COMPTH_80, ADS_LOFF_I_24nA, ADS_LOFF_F_31_2HZ);

// Alias de compatibilidad: mantiene el comportamiento probado de la librería.
static constexpr uint8_t ADS_LOFF_DCAC_24nA_31Hz_80pct =
  ADS_LOFF_AC_24NA_31HZ_87_5PCT_LEGACY;

// =========================
// CHnSET (0x05..0x0C) — 9.6.1.6 (canales)
// =========================
// [7]=PDn, [6:4]=GAIN, [3]=SRB2, [2:0]=MUX
#define ADS_CH_PD 0x80

enum : uint8_t {
  ADS_GAIN_1  = 0b000,
  ADS_GAIN_2  = 0b001,
  ADS_GAIN_4  = 0b010,
  ADS_GAIN_6  = 0b011,
  ADS_GAIN_8  = 0b100,
  ADS_GAIN_12 = 0b101,
  ADS_GAIN_24 = 0b110
};

enum : uint8_t {
  ADS_MUX_NORMAL    = 0b000,
  ADS_MUX_SHORT     = 0b001,
  ADS_MUX_BIAS_MEAS = 0b010,
  ADS_MUX_MVDD      = 0b011,
  ADS_MUX_TEMP      = 0b100,
  ADS_MUX_TESTSIG   = 0b101,
  ADS_MUX_BIASP     = 0b110,
  ADS_MUX_BIASN     = 0b111
};

#define ADS_CH_SRB2 0x08
#define ADS_CH_MAKE(on, gain3b, mux3b, srb2) \
  (uint8_t)(((on) ? 0 : ADS_CH_PD) | (((gain3b) & 0x07) << 4) | ((srb2) ? ADS_CH_SRB2 : 0) | ((mux3b) & 0x07))

inline uint8_t ADS_CH_DEFAULT_GAIN24() {
  return ADS_CH_MAKE(true /*on*/, ADS_GAIN_24, ADS_MUX_NORMAL, false /*srb2*/);
}

// =========================
// Máscaras de canal
// =========================
#define ADS_MASK_CH1 0x01
#define ADS_MASK_CH2 0x02
#define ADS_MASK_CH3 0x04
#define ADS_MASK_CH4 0x08
#define ADS_MASK_CH5 0x10
#define ADS_MASK_CH6 0x20
#define ADS_MASK_CH7 0x40
#define ADS_MASK_CH8 0x80

static inline uint8_t ADS_ClipMaskToChannels(uint8_t mask, uint8_t nchan) {
  static const uint8_t lut[9] = {0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF};
  if (nchan > 8) nchan = 8;
  return (uint8_t)(mask & lut[nchan]);
}

// =========================
// LOFF_SENSP / LOFF_SENSN (0x0F/0x10) — 9.6.1.9/10
// =========================
#define ADS_LOFF_SENS_MASK(chMask) ((uint8_t)(chMask))

// =========================
// LOFF_FLIP (0x11) — 9.6.1.11
// =========================
#define ADS_LOFF_FLIP_MASK(chMask) ((uint8_t)(chMask))

// =========================
// LOFF_STATP / LOFF_STATN (0x12/0x13) — 9.6.1.12/13 (R)
// =========================
inline bool ADS_IsLeadOffP(uint8_t statP, uint8_t ch) {
  return (statP >> (ch - 1)) & 0x01;
}

inline bool ADS_IsLeadOffN(uint8_t statN, uint8_t ch) {
  return (statN >> (ch - 1)) & 0x01;
}

// =========================
// GPIO (0x14) — 9.6.1.14
// =========================
// [7:4]=GPIOD, [3:0]=GPIOC. GPIOC: 1=input, 0=output.
#define ADS_GPIO_DIR_IN_ALL   0x0F
#define ADS_GPIO_DIR_OUT_ALL  0x00
#define ADS_GPIO_MAKE(data4, dir4) \
  (uint8_t)((((data4) & 0x0F) << 4) | ((dir4) & 0x0F))

static constexpr uint8_t ADS_GPIO_ALL_INPUTS =
  ADS_GPIO_MAKE(0x0, ADS_GPIO_DIR_IN_ALL);

// =========================
// MISC1 (0x15) — 9.6.1.15
// =========================
#define ADS_MISC1_SRB1_EN 0x20
#define ADS_MISC1_SRB1    ADS_MISC1_SRB1_EN

// =========================
// CONFIG4 (0x17) — 9.6.1.17
// =========================
// [3]=SINGLE_SHOT, [1]=PD_LOFF_COMP.
// Tabla de campo del datasheet: bit1=0 comparadores lead-off disabled,
// bit1=1 comparadores lead-off enabled.
#define ADS_CFG4_SINGLE_SHOT    0x08
#define ADS_CFG4_PD_LOFF_COMP   0x02  // nombre del datasheet
#define ADS_CFG4_LOFF_COMP_EN   ADS_CFG4_PD_LOFF_COMP
#define ADS_CFG4_CONT_CONV      0x00

static constexpr uint8_t ADS_CFG4_CONT_LOFF_COMP_OFF = ADS_CFG4_CONT_CONV;
static constexpr uint8_t ADS_CFG4_CONT_LOFF_COMP_ON  = ADS_CFG4_CONT_CONV | ADS_CFG4_LOFF_COMP_EN;

// Alias legibles.
static constexpr uint8_t ADS_CFG4_CONT_LOFF_OFF = ADS_CFG4_CONT_LOFF_COMP_OFF;
static constexpr uint8_t ADS_CFG4_CONT_LOFF_ON  = ADS_CFG4_CONT_LOFF_COMP_ON;

// =========================
// STATUS (24 bits al inicio de cada frame RDATAC/RDATA) — 9.4.4.2
// =========================
// STATUS[23:20] = 1100b
// STATUS[19:12] = LOFF_STATP (ch8..ch1)
// STATUS[11:4]  = LOFF_STATN (ch8..ch1)
// STATUS[3:0]   = GPIO[4:1]
#define ADS_STATUS_SYNC_MASK   0xF00000u
#define ADS_STATUS_SYNC_VAL    0xC00000u
#define ADS_STATUS_LOFFP(s)    (uint8_t)(((s) >> 12) & 0xFF)
#define ADS_STATUS_LOFFN(s)    (uint8_t)(((s) >> 4) & 0xFF)
#define ADS_STATUS_GPIO4_1(s)  (uint8_t)((s) & 0x0F)
