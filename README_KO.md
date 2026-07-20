# C99-6502

[English / 영어](./README.md)

<img width="3000" height="1000" alt="image" src="https://github.com/user-attachments/assets/93fb6303-551e-42a6-aa75-63211b8c0d91" />

2학년 학부생이 듣기에 시스템 소프트웨어 강의가 쉬운 건... 아닌 것 같습니다. 그래서, 조금이나마 더 쉽게 이해하고자 널리 알려진 마이크로프로세서의 에뮬레이터를 직접 제작하며 운영 체제의 구조를 익히고자 합니다.

## 소개

<img width="1031" height="451" alt="image" src="https://github.com/user-attachments/assets/7dde66e5-c826-4ad2-b063-2c73f3932f6c" />

*TL;DR: C99-6502는 MOS 6502의 NMOS 6502와 CMOS 65C02 변형의 에뮬레이션을 모두 지원하는, ISO/IEC 9899:1999 표준을 준수하는 C로 작성된 에뮬레이터입니다. POSIX 표준을 준수하는 환경에서는 추가적인 편의 기능이 지원됩니다.*

본 프로젝트는 사이클 정밀도를 보장하는 MOS 6502 마이크로프로세서와 그 변형을 에뮬레이팅하는 C99로 작성된 에뮬레이터로, 설계 문서에 포함된 작동 특성과 타이밍 관련 요점을 포함한 원 NMOS 6502 프로세서의 모든 작동을 섬세하게 모방하는 것이 특징입니다.

본 에뮬레이터는 개발자 문서에 서술된 명령어 세트 전체에 더하여 흔히 쓰이는 안정적으로 동작하는 비공식 opcode들까지 지원하며, page-crossing과 분기 과정에서 발생하는 시간적 지연을 포함하여 사이클 측정을 구현하였습니다. `JMP ($xxFF)` 래핑 버그, 제로페이지 주소 래핑, 그리고 NMOS의 십진수 모드 플래그 세멘틱과 같은 하드웨어 특성이 정밀하게 구현되었습니다. 코드베이스는 버스 인터페이스, 지역 기반의 메모리 관리, CPU 코어, 주소 지정 방식, 명령어 실행, 스택 명령, 그리고 실행 트레이스를 다루는 모듈로 구성됩니다.

메모리의 구성은 주소가 RAM, ROM, 그리고 입출력 영역으로 구분되어 있는 지역 기반 시스템을 사용합니다. 기본 구성의 경우, 32KiB RAM ($0000-$7FFF)과 32KiB ROM ($8000-$FFFF)을 할당함으로서 다양한 클래식 6502 시스템의 구성을 모방합니다. ROM 영역은 자동으로 쓰기 보호가 적용되며, 입출력 영역은 디바이스 에뮬레이션을 위한 커스텀 읽기/쓰기 핸들러를 지원합니다. 이 유연한 아키텍처는 NES, Apple II, Commodore 64와 같은 여러 시스템의 적절한 메모리 매핑을 구성할 수 있게 함으로서 다양한 시스템의 정확한 에뮬레이션을 가능케 합니다.

본 에뮬레이터는 NMOS 6502와 CMOS 65C02 CPU 두 가지를 모두 지원합니다. CPU의 종류는 에뮬레이터 실행 시 명령줄 구문을 통하여 선택될 수 있으며, (미선택 시 NMOS 6502) BCD 플래그의 작동, JMP 래핑 버그의 해결, 명령 세트 추가 등과 같은 65C02와 6502 간 차이점을 구현하였습니다.

CMOS 65C02의 경우, `BRA`, (무조건 분기), `PHX`/`PHY` (X/Y 레지스터 푸시), `PLX`/`PLY` (X/Y 레지스터 풀), `STZ` (제로 저장), `TRB`/`TSB` (비트 테스트 및 리셋/셋), `WAI` (인터럽트 대기), `STP` (프로세서 정지) 와 같은 기존의 명령뿐만 아니라, `RMB0-7` (메모리 비트 리셋), `SMB0-7` (메모리 비트 셋), `BBR0-7` (비트 리셋 시 분기), `BBS0-7` (비트 셋 시 분기) 등지의 Rockwell/WDC 65C02 비트 Manipulation 명령어 또한 구현되었습니다.

또한, ISO/IEC 9899:1999 표준에 포함되는 요소가 아니하나, 본 에뮬레이터의 편리한 사용의 지원을 위하여 POSIX 표준 준수 환경에서 사용 가능한 `ncurses` 라이브러리 기반의 TUI CPU 시각화 모니터, Python 3을 통한 복합적 테스트 ROM 빌드와 같이 선택적으로 사용 가능한 비표준 요소를 포함하고 있습니다.

### 빠른 시작

```bash
# 전체 빌드 (POSIX 환경 하)
make

# ISO/IEC 9899:1999 표준 준수 빌드
make main

# 예시 ROM 실행
make run

# Trace 활성화 후 ROM 실행
make run-trace
```

## 요구 사항

### 필수

| 요구 사항               | 설명                                             |
| ------------------------- | ------------------------------------------------------- |
| ISO/IEC 9899:1999 표준을 준수하는 컴파일러    | *예: GCC 또는 Clang*                                    |

### 선택

| 요구 사항               | 설명                                             |
| ------------------------- | ------------------------------------------------------- |
| make, 또는 호환되는 빌드 자동화 도구    | 빌드 자동화 도구                                    |
| ncurses                   | TUI 모니터용 프레임워크                               |
| POSIX.1-2008 표준 충족 환경 | `clock_gettime`를 통한 비-POSIX 환경 대비 더욱 정확한 시간 측정 |
| Python 3                  | 예제 ROM 빌드를 위한 도구                             |

*참고: ncurses는 대부분의 POSIX 표준을 충족하는 환경에 포함되어 있습니다.*

## 빌드

```bash
# 에뮬레이터 전체의 빌드 (make posix와 동등)
make

# POSIX 환경 하 지원 편의 기능이 포함된 에뮬레이터 전체의 빌드
make posix

# ISO:IEC 9899:1999 표준을 만족하는, 편의 기능 제외 순수 에뮬레이터 빌드
make main

# TUI 모니터의 빌드
make monitor

# 예제 ROM의 빌드
make rom

# 예제 ROM 실행
make run

# 예제 ROM 실행 시 명령어별로 실행을 추적
make run-trace

# 검증 테스트 빌드 및 실행
make verify

# 빌드된 파일 정리
make clean
```

*참고: GNU make 또는 호환되는 빌드 자동화 도구가 필요합니다.*

## 실행

### 명령행 인자

| 옵션               | 설명                                             |
| -------------------- | ------------------------------------------------------- |
| `-f`, `--file`       | 바이너리 파일의 로드                                     |
| `-a`, `--address`    | 로드 주소 *(16진수, e.g., `8000`, `C000`)*              |
| `-c`, `--cpu`        | CPU 유형 *(`6502`, `nmos`, `65c02`, `cmos`, 기본 `nmos`)* |
| `-t`, `--trace`      | 명령 단위 Trace 실행                                     |
| `-m`, `--monitor`    | TUI 모니터 실행 *(ncurses 필요)*      |
| `-r`, `--ram-start`  | RAM 시작 주소 *(16진수, 기본 `0000`)*           |
| `-R`, `--ram-size`   | RAM 크기 *(Byte 단위, 기본 `32768`)*                 |
| `-s`, `--rom-start`  | ROM 시작 주소 *(16진수, 기본 `8000`)*           |
| `-S`, `--rom-size`   | ROM 크기 *(Byte 단위, 기본 `32768`)*                 |

#### 예제

```bash
# $8000 상 바이너리 로드
bin/mos6502 -f program.bin -a 8000

# 명령 단위 Trace 실행
bin/mos6502 -f program.bin -a C000 -t

# CMOS 65C02 사용
bin/mos6502 -f program.bin -a 8000 -c 65c02

# TUI 모니터 실행
bin/mos6502 -f program.bin -a 8000 -m

# 메모리 레이아웃 설정 (16KiB RAM + 16KiB ROM)
bin/mos6502 -r 0x0000 -R 16384 -s 0x4000 -S 16384

# 여러 옵션의 결합
bin/mos6502 -f program.bin -a 8000 -t -c 65c02 -r 0x0 -R 32768
```

### TUI 모니터

*TL;DR: 이 에뮬레이터는 시각화, 디버깅, 분석과 같은 작업을 지원하기 위한 다양한 기능을 가진 텍스트 기반의 대화형 모니터를 포함합니다.*

에뮬레이터는 실시간 시각화 및 디버깅을 위한 대화형 TUI (텍스트 사용자 인터페이스) 모니터를 포함합니다. `-m` 또는 `--monitor` 플래그로 실행할 수 있습니다.

*참고: 본 추가 기능의 사용에는 ncurses 라이브러리가 필요합니다.*

#### 기능

| 기능 | 설명 |
| --- | --- |
| 상태 감시 | 레지스터와 플래그 상태의 확인 |
| 실시간 역어셈블리 | 차기 실행 명령의 확인 |
| 실행 기록 | 최근 실행 명령의 확인 |
| 성능 통계 | 사이클 카운트와 프레임 타이밍의 분석 |
| 메모리 시각화 | ASCII 변환 포함 메모리 hexdump |
| 실시간 디버깅 | 브레이크포인트, 워치포인트, 하드웨어 기반 상태 조정이 포함된 단계별 디버깅 도구 |

#### 조작

| 키 | 설명 |
| --- | ------ |
| `S` | Step - 하나의 명령 실행 |
| `C` | Continue - 실행 재개 |
| `B` | Break - 실행 정지 |
| `R` | Reset - CPU 상태 초기화 |
| `Q` | Quit - 모니터 종료 |

#### 디스플레이 패널

본 모니터는 여섯 가지의 주요 패널로 구성되어 있습니다.

| 패널 | 설명 |
| --- | --- |
| 상태 | CPU 상태 (실행/정지), 속도, 사이클 카운트, 업타임, 메모리 구성 |
| 레지스터 | `PC`, `SP`, `A`, `X`, `Y`, `P`, CPU 플래그 분해 (`N` `V` - `B` `D` `I` `Z` `C`) |
| 역어셈블리 | 현 프로그램 카운터 이후 실행 예정 명령 |
| 메모리 | ASCII 형식 표현 변환 포함 메모리 영역의 Hexdump |
| 명령 | 레지스터 상태 포함 실행 기록 |
| 성능 | 실시간 성능 지표 (현재, 평균, 1% low, 0.1% low) |

*참고: TUI 모니터의 실행을 위해서는, 최소 80열 x 19행을 출력할 수 있는 터미널 크기가 필요합니다.*

## 설계 오버뷰

*TL;DR: 아키텍처는 모듈 형식으로 설계되었습니다. 따라서, 각각의 서브시스템은 CPU 작동의 각각의 고유한 부분을 처리합니다.*

본 에뮬레이터는 여러 핵심 모듈들을 중심으로 구성되어 있습니다. CPU 모듈(`cpu.c/h`)은 사이클 측정과 인터럽트 핸들링 (`IRQ`/`NMI`/`Reset`)을 포함한 핵심 인출/해석/실행 루프를 구현합니다. 메모리 접근은 버스 인터페이스(`bus.c/h`)를 통해 읽기/쓰기 함수 포인터로 구현되며, 이로 인해 여러 장치가 주소 공간에 매핑될 수 있습니다. 메모리 모듈(`memory.c/h`)은 주소 공간을 RAM, ROM, 그리고 입출력 영역으로 구분하는 지역 기반 시스템을 구현합니다. RAM은 읽기/쓰기가 가능하고, ROM은 읽기 전용이며 (쓰기 보호 처리됨), 입출력 영역은 디바이스 에뮬레이션을 위한 커스텀 읽기/쓰기 핸들러를 지원하는 것과 같이 각각의 영역은 다른 속성을 지닐 수 있습니다.

모든 6502 주소 지정 방식은 정밀한 페이지 크로스 감지를 포함하여 `addressing.c/h`에 구현되어 있습니다. 명령어 시스템은 256개의 엔트리를 가진, 메타데이터가 포함된 디스패처 테이블을 사용하며, 각각의 핸들러가 명령 세만틱과 타이밍을 구현합니다. 스택 연산(`stack.c/h`)은 $0100–$01FF 범위에서의 stack을 구현하며, 8-bit, 그리고 16-bit push/pop 기능을 지원합니다. 추가 모듈들은 실행 트레이싱과 바이너리의 로딩을 지원합니다.

## 메모리 맵

기본적으로 32KiB를 RAM으로, 32KiB을 ROM으로 할당합니다.

|         위치 | 유형 | 용도                                       |
| ----------: | ---- | ------------------------------------------ |
| $0000–$00FF | RAM  | 제로 페이지                                  |
| $0100–$01FF | RAM  | 스택                                      |
| $0200–$7FFF | RAM  | 범용 RAM                                |
| $8000–$FFF9 | ROM  | 프로그램 코드와 데이터                      |
| $FFFA–$FFFB | ROM  | NMI 벡터                                 |
| $FFFC–$FFFD | ROM  | 리셋 벡터                               |
| $FFFE–$FFFF | ROM  | IRQ/BRK 벡터                             |

이 메모리 레이아웃은 명령줄 옵션 (`--ram-start`, `--ram-size`, `--rom-start`, `--rom-size`) 과 지역 기반 API를 통하여 구성될 수 있으며, 이를 통하여 여러 6502 기반 시스템을 구현하는 범용 에뮬레이터로 확장될 수 있습니다.

### 메모리 영역의 구성

본 에뮬레이터는 지역 기반의 메모리 시스템을 사용하여 다음과 같은 유연한 구성을 지원합니다:

```c
// 존재하는 메모리 영역 삭제
mem_region_clear();

// $0000-$7FFF 주소에 RAM 할당
mem_region_add_ram(0x0000, 0x8000);

// $8000-$FFFF 주소에 ROM 할당
mem_region_add_rom(0x8000, 0x8000);

// 메모리에의 ROM 파일 적재
load_bin_region("program.bin", 0x8000);

// 리셋 벡터의 설정
mem_region_set_vector(0xFFFC, 0x8000);

// 버스 시스템의 init
mem_region_init();
```

I/O 매핑된 장치의 경우, 커스텀 읽기/쓰기 핸들러와 같이 `mem_region_add_io()` 를 사용하십시오:

```c
MEM_WORD io_read(MEM_TWO_WORDS addr, void* ctx) {
    // 커스텀 I/O 읽기
    return 0x00;
}

void io_write(MEM_TWO_WORDS addr, MEM_WORD data, void* ctx) {
    // 커스텀 I/O 쓰기
}

// $6000-$6FFF 영역에 I/O 매핑
mem_region_add_io(0x6000, 0x1000, io_read, io_write, NULL);
```

ROM 영역은 자동적으로 쓰기 보호되며, ROM 주소로의 쓰기는 실 하드웨어와 동일하게 경고 없이 무시됩니다.

## 설계

### 정확성에 대해

*TL;DR: 모든 CPU 명령은 페이지 크로싱에서 유발된 시간 지연과 분기 딜레이를 포함하여 실 하드웨어와 동일한 클럭 사이클을 소모하여 에뮬레이션됩니다. 또한, 그러한 하드웨어 특성들 또한 에뮬레이션됩니다.*

타이밍은 원 하드웨어의 사양과 동일한 일개 명령당 베이스 사이클 카운트를 사용합니다. 인덱싱된 읽기 연산에서의 (ABSX/ABSY/INDY) 페이지 크로싱, 수행된 분기, 그리고 분기 실행 중 발생한 페이지 경계 초과의 경우 사이클이 추가적으로 가산됩니다.

명령 타이밍과 NMOS의 고유 하드웨어 특성 또한 사이클 단위 정밀도로 원 마이크로프로세서를 모방합니다. 제로 페이지 인덱스 주소 지정 모드의 `$00FF` 경계에서의 래핑, 또는 `(IND,X)` 포인터 계산에서의 래핑 등 요소가 포함되어 있습니다. NMOS와 CMOS간 동작 차이에 대해서는, 차기 목차를 참조하십시오.

### CPU간 차이

*TL;DR: NMOS와 CMOS 65C02는 몇 가지 작지만 중요한 점에서 다르게 구동됩니다. 본 에뮬레이터는 두 CPU의 에뮬레이션을 모두 지원합니다.*

본 에뮬레이터는 NMOS 6502와 CMOS 65C02의 에뮬레이션을 다음과 같이 지원합니다:

| 영역 | NMOS 6502 | CMOS 65C02 |
| ------ | --------- | ----------- |
| BCD (십진수) 모드 플래그 | `N`, `Z` 플래그는 BCD 연산 이전의 바이너리 결과를 반영합니다; `V` 플래그의 경우 바이너리 연산 이후의 결과를 반영합니다. | `N`, `Z` 플래그는 BCD 연산 이후의 결과를 반영합니다; `V` 플래그의 경우 바이너리 연산 이후의 결과를 반영합니다. |
| `$xxFF` 에서의 간접 `JMP` | `JMP ($xxFF)` 는 페이지 내에서 래핑되며, `$xx00` 의 high byte를 같은 페이지에서 읽습니다. | 페이지 크로싱 문제가 수정되었습니다; `JMP ($xxFF)` 의 경우 다음 페이지에서 high byte를 읽습니다. |
| 명령어 세트 | 기본 6502 명령어 세트 (56개의 공식 명령 / 151개의 공식 opcode)과 안정적으로 작동하는 비공식 MOS 6502 명령어 일부 포함 (`LAX`, `SAX`, `DCP`, `ISC`, `SLO`, `RLA`, `SRE`, `RRA`). | 기본 6502 명령어 세트 + 10개의 신규 CMOS 65C02 명령: `BRA` (무조건 분기), `PHX/PHY` (X/Y 푸시), `PLX/PLY` (X/Y 풀), `STZ` (`0` 저장), `TRB`/`TSB` (비트 테스트 및 Reset / Set), `WAI` (Interrupt 대기), `STP` (프로세서 정지). 32개의 Rockwell/WDC 비트 연산  `RMB0-7` (메모리 비트 리셋), `SMB0-7` (메모리 비트 셋), `BBR0-7` (비트 리셋 시 분기), `BBS0-7` (비트 셋 시 분기) / 비공식 명령의 경우 `NOP`로 처리 |
| 변형 검사 | 명령은 변형 검사 없이 실행됩니다. | 65C02 전용 명령은 CPU가 65C02 모드일 때만 실행되며, NMOS 모드에서는 NOP로 처리됩니다. |

*참고: 두 경우 모두 올림 플래그 (`C`)와 오버플로우 플래그 (`V`)는 동일하게 작동합니다.*

### 인터럽트

본 에뮬레이터는 6502 인터럽트 동작을 정확하게 모델링하는 포괄적인 인터럽트 컨트롤러를 포함합니다.

#### 기능

| 기능                        | 설명                                                      |
|---------------------------------|------------------------------------------------------------------|
| 소프트웨어 인터럽트 (BRK)        | BRK 명령 실행, B 플래그 설정, IRQ/BRK 벡터 사용                         |
| 마스크 가능 인터럽트 (IRQ)        | 레벨 트리거, I 플래그(인터럽트 비활성화) 존중                               |
| 마스크 불가능 인터럽트 (NMI)    | 엣지 트리거(하강 엣지), 항상 실행, NMI 벡터 사용                           |
| 엣지 감지                  | NMI는 1→0 전환(하강 엣지)에서만 트리거                                    |
| 인터럽트 우선 순위              | NMI가 더 높은 우선순위, IRQ 시퀀스를 가로챌 수 있음                         |
| 인터럽트 기록              | 디버깅을 위해 마지막 32개 인터럽트 기록                                    |
| 통계 추적             | 프로파일링을 위한 총 IRQ, NMI, BRK 수 카운트                              |
| 정밀한 타이밍 시퀀스        | 실제 6502 하드웨어를 모델링하는 7사이클 인터럽트 시퀀스                       |

#### 인터럽트 목록

| 타입 | 트리거 | 마스크 | 벡터 | B 플래그 |
|------|---------|----------|--------|--------|
| `BRK` | 소프트웨어 | 불가능 | `$FFFE` | 세트 (`1`) |
| `IRQ` | 레벨 | 가능 (`I` 플래그) | `$FFFE` | 클리어 (`0`) |
| `NMI` | 하강 엣지 | 불가능 | `$FFFA` | Clear (`0`) |

#### 사용 예시

```c
#include "interrupt.h"

// 인터럽트 컨트롤러 초기화
cpu_init();  // 자동 인터럽트 초기화

// IRQ (레벨 트리거)
interrupt_set_irq(1);  // IRQ High로 설정
// I 플래그의 클리어가 만족되면 CPU는 ISR을 수행

// NMI (엣지 트리거)
interrupt_set_nmi(1);  // NMI High로 설정
interrupt_set_nmi(0);  // 하강 엣지 설정 → NMI 트리거

// 대기 중인 인터럽트의 확인
INTERRUPT_TYPE type = interrupt_poll();
if (type == INT_NMI) {
    // NMI 인터럽트가 대기 중
}

// 인터럽트 기록 추적
interrupt_dump_history();
interrupt_dump_stats();
```

#### 인터럽트 테스트의 수행

```bash
make interrupt-test
./bin/interrupt_test
```

인터럽트 테스트는 BRK, IRQ, NMI, 엣지 감지, 우선순위 처리 및 히스토리 추적을 시연합니다.

### 지원 명령

모든 공식 6502 명령이 구현되었습니다. NMOS 모드로 실행하는 경우, 실제 NMOS 시스템에서 사용된 비공식 명령을 지원하며, 대표적인 예시는 다음과 같습니다:`LAX`, `SAX`, `DCP`, `ISC`, `SLO`, `RLA`, `SRE`, `RRA`. 다만 일부 불안정한 비공식 명령 (`$9B`, `$9C`, `$9E`, `$9F`)은 실 하드웨어의 예측 불가능한 작동으로 인하여 제외되었습니다.

65C02 모드에서는 모든 표준 WDC 65C02 명령과 *완전한 Rockwell/WDC 비트 조작 확장 명령*을 지원합니다:

`BRA`, `PHX`, `PHY`, `PLX`, `PLY`, `STZ` (4 addressing modes), `TRB`, `TSB`, `WAI`, `STP`, *`RMB0-7`, `SMB0-7`, `BBR0-7`, `BBS0-7` (총 32개 명령)*

*참고: 65C02 모드에서, 대부분의 비공식 명령들은 NOP으로 치환되었습니다. 현재 구현상 해당 경우 두 모드 모두에서 NOP로 간주되는데, 65C02 관점에서 기능상 옳으나 몇 가지의 NMOS 6502에서만 동작하는 비공식 명령이 작동하지 않을 가능성이 있습니다.*

## 디버깅 및 프로파일링 도구

에뮬레이터는 프로그램 실행을 분석하고, 버그를 찾고, 코드를 최적화하는 데 도움이 되는 포괄적인 디버깅 및 프로파일링 시스템을 포함합니다. 디버깅 기능은 `debugger.h` 라이브러리를 통해 제공됩니다. 주요 기능은 다음과 같습니다:

### 기능 개요

| 기능              | 설명                                                                 |
|----------------------|-----------------------------------------------------------------------------|
| 중단점          | 특정 주소에서 실행, 메모리 읽기, 쓰기 또는 접근에 대한 중단점 설정                          |
| 워치포인트          | 특정 메모리 위치의 변경 사항을 추적하고 알림 수신                                        |
| 사이클 프로파일링      | 총 사이클 수를 측정하고 명령별 실행 빈도 분석                                           |
| 핫스팟 분석     | 가장 자주 실행되는 코드 영역 감지                                                    |
| 메모리 감시    | hex dump, ASCII 표현 및 역어셈블리를 통한 메모리 내용 확인                                  |
| 스택 검사     | 스택의 현재 상태 및 내용 분석                                                        |
| 메모리 검색        | 전체 메모리 공간에서 특정 바이트 패턴 검색                                              |
| 레지스터 검사  | 플래그 정보를 포함한 전체 CPU 상태 검사                                               |

### 사용 예시

```c
#include "debugger.h"

// 디버거 초기화
debugger_init();

// 중단점 설정
debugger_add_breakpoint(BP_TYPE_EXEC, 0x8000, "main_loop");
debugger_add_breakpoint(BP_TYPE_WRITE, 0x0200, "output_port");

// 워치포인트 추가
debugger_add_watchpoint(0x0200, "counter_variable");

// 실행 도중 중단점 확인
if (debugger_check_breakpoint(BP_TYPE_EXEC, REG.PC)) {
    debugger_dump_registers(); // CPU 상태 덤프
}

// 프로파일링 수행
profiler_record_instruction(pc, opcode, cycles);
profiler_dump_stats();           // 명령 빈도 출력
profiler_dump_hotspots(10);      // 빈도가 상위 10위 내 포함되는 명령 출력

// 메모리 검사
debugger_hexdump(0x8000, 256);        // ASCII 변환 포함 Hexdump
debugger_disassemble(0x8000, 20);     // 명령 역어셈블리
debugger_dump_stack();                // 스택의 현재 상태 및 내용 분석
debugger_dump_registers();            // 레지스터 및 플래그 정보 출력

// 메모리 검사
MEM_WORD pattern[] = { 0xA9, 0x42 };  // LDA #$42
debugger_search_memory(0x8000, 0xFFFF, pattern, 2);

debugger_cleanup(); // Cleanup
```

### 디버거 테스트

```bash
make debug-test
./bin/debug_test
```

디버그 테스트는 루프, 메모리 수정 및 산술 연산을 수행하는 샘플 프로그램과 함께 모든 디버깅 기능을 시연합니다.

## 테스팅

에뮬레이션의 구현 정확도를 검증하기 위한 테스팅 수단이 포함되어 있습니다. `tests` 폴더를 참조하십시오.

### Klaus Dormann Functional Tests

에뮬레이터는 모든 공식 명령, 주소 지정 모드, 플래그 작동을 포함한 MOS 6502의 작동을 검증하는 대표적인 커뮤니티 제작 테스트인 Klaus Dormann 6502 기능 테스트를 통과합니다. 테스트는 약 30.6M 사이클 내에 성공적으로 완료됩니다.

```bash
# Test suite 초기 설정
make download-functional-test

# 기능 테스트 빌드 및 실행
make functional-test
bin/functional_test

# CMOS 65C02 테스트
bin/functional_test -c 65c02
```

본 테스트의 경우 (`tests/6502_functional_test/run_functional_test.c`)는 64KiB의 RAM을 구성하고 테스트 바이너리를 로드한 후, 프로그램 카운터를 모니터링하여 테스트 성공 또는 실패를 감지합니다. 테스트 진행 상황은 백만 사이클당 한 개의 점(`.`)으로 표시됩니다.

### 기본 검증 테스트

본 에뮬레이터는 호스트 도구를 포함하여, 6502와 65C02의 작동을 시험하는 테스트를 *(약칭 verification test suite)* `tests/minimal/verify_test.c`에 포함하고 있습니다.

```bash
# NMOS 6502 검증
make verify
bin/mos6502 -f tests/minimal/test.bin -a 8000 -t

# CMOS 65C02 검증
make verify-65c02
bin/verify_65c02_test
```

6502 검증 테스트에 더해, 65C02 검증 테스트의 경우 `BRA`, `PHX`, `PHY`, `PLX`, `PLY`, `STZ`, `TSB`, `TRB`와 같은 CMOS 65C02 명령어 또한 검증합니다.

*참고: 65C02 검증 테스트의 수행에는 Python 3이 필요합니다.*

## 제한 사항

본 프로젝트는 PPU, APU, 키보드와 같은 외부 장치의 구현이 제외되고 CPU에 중점을 둔 에뮬레이터로, 구성 가능한 I/O 영역과 커스텀 핸들러를 통하여 장치 에뮬레이션의 기반을 지원하나, 현재로선 특정 장치가 구현되지 않았습니다. 고도로 불안정한 비공식 명령은(`$9B`, `$9C`, `$9E`, `$9F` 등) 실 하드웨어의 예측 불가능한 작동으로 인하여 구현 대상에서 제외되었습니다.

## 코딩 컨벤션

본 코드베이스는 다음과 같은 네이밍 컨벤션을 따릅니다: 전역에서 접근 가능한 경우에는 ALL_CAPS의 형식으로 명명되었으며(예: `REG`, `BUS`, `EA`), typedef의 경우 `T_` 접두사를 가지고(예: `T_REGISTER`), 상수 또는 매크로의 경우 ALL_CAPS의 형식으로 명명됩니다(예: `FLAG_C`). 코드는 반각 공백 문자 4개로 들여쓰여지며, `.h` 파일에서 선언이, `.c` 파일에서 구현이 이루어지는 표준 관행 또한 프로젝트 전체에서 준수됩니다.

## 프로젝트 레이아웃

```Text
.
├── src/                      # Core sources
│   ├── addressing.c
│   ├── bus.c
│   ├── cpu.c
│   ├── debugger.c            # 디버깅 및 프로파일링
│   ├── instructions_handlers.c        # Opcode 디스패치
│   ├── instructions_implementation.c  # Opcode 핸들러
│   ├── instructions_table.c           # Opcode 메타데이터
│   ├── interrupt.c           # 인터럽트 컨트롤러
│   ├── loader.c
│   ├── logging.c             # 로깅 시스템
│   ├── memory.c
│   ├── stack.c
│   ├── trace.c
│   └── tui_monitor.c         # TUI 모니터 구현
├── tests/                    # Test suites
│   ├── 6502_functional_test/ # Klaus Dormann 기능 테스트
│   ├── minimal/              # 최소 및 65C02 테스트 스위트
│   ├── verify_test.c         # 기본 검증
│   ├── debug_test.c          # 디버거 테스트
│   ├── interrupt_test.c      # 인터럽트 테스트
│   ├── simple_debug_test.c   # 단순 디버거 테스트
│   └── undocumented_test.c    # 비공식 opcode 테스트
├── include/                  # Header files
│   ├── addressing.h
│   ├── bus.h
│   ├── cpu.h
│   ├── debugger.h
│   ├── instructions_handlers.h
│   ├── instructions_implementation.h
│   ├── instructions_table.h
│   ├── interrupt.h
│   ├── loader.h
│   ├── logging.h
│   ├── memory.h
│   ├── stack.h
│   ├── trace.h
│   ├── tui_monitor.h         # TUI 모니터 헤더
│   └── types.h
├── bin/                      # Build outputs
├── main.c
├── Makefile
├── README.md
├── README_KO.md              # 한국어 버전 README
└── .gitignore
```

## 참고 사항

본 에뮬레이터는 MOS Technology 6502 Programming Manual, [Masswerk의 6502 참고 자료](https://www.masswerk.at/6502/)의 문서 및 [Visual 6502](https://visual6502.org)의 트랜지스터 단위 구현 프로젝트를 기반으로 설계, 작성, 구현되었습니다.

## 라이센스

본 프로젝트는 GNU Affero General Public License Version 3.0 하 보호됩니다. 자세한 라이센스 관련 정보는 [LICENSE](LICENSE)를 참조하십시오.

### 오픈소스 라이센스

[tests/6502_functional_test](tests/6502_functional_test) 폴더 내에는 Klaus Dormann이 제작한 [6502_functional_test](https://github.com/Klaus2m5/6502_65C02_functional_tests)과 본 프로젝트의 관련 파일이 포함되어 있으며, 해당 폴더 내의 파일은 GNU General Public License Version 3.0 하 보호됩니다. 해당 라이센스 관련 정보는 [해당 폴더 내의 README](tests/6502_functional_test/README_KO.md), 그리고 [해당 폴더 내의 LICENSE](tests/6502_functional_test/LICENSE)를 참조하십시오.

## 또한

끝까지 읽어 주셔서 감사합니다! 좋은 하루 되세요 (˶˃ ᵕ ˂˶) .ᐟ.ᐟ

<sub>*METHYLPHENIDATE로 구동되지만 LISDEXAMFETAMINE이 필요하나 국내에서 불법이기에 MALADAPTIVE DAYDREAMING으로 고통받는, 강의를 어떻게든 이해하고자 몸부림치는 2학년 학부생이 제작하였습니다. 새벽 1시, UTC+9. ~~제발 누가 날 한국에서 꺼내줘요~~*</sub>