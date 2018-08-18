//
//  SpaceInvadersMachine.m
//  SpaceInvadersEMU
//
//  Created by mcjack on 8/17/18.
//  Copyright © 2018 Jack McCluskey. All rights reserved.
//
#define USE_THREADS 1

#import "SpaceInvadersMachine.h"
#include <sys/time.h>

@implementation SpaceInvadersMachine

// Code courtesy of Kent Miller and emulator101.com
// I am not super familiar with Cocoa setups right now
-(void) ReadFile:(NSString *)filename IntoMemoryAt:(uint32_t)memOffset {
    NSBundle *mainbundle = [NSBundle mainBundle];
    NSString *filepath = [mainbundle pathForResource:filename ofType:NULL];
    
    NSData *data = [[NSData alloc]initWithContentsOfFile:filepath];
    
    if (data == NULL)
    {
        NSLog(@"Open of %@ failed.  This is not going to go well.", filename);
        return;
    }
    if ([data length] > 0x800)
    {
        NSLog(@"corrupted file %@?  shouldn't be this big: %ld bytes", filename, [data length]);
    }
    
    uint8_t *buffer = &state->memory[memOffset];
    memcpy(buffer, [data bytes], [data length]);
}

-(id) init {
    state = calloc(sizeof(state8080), 1);
    state->memory = malloc(16 * 0x1000);
    
    [self ReadFile:@"invaders.h" IntoMemoryAt:0x0000];
    [self ReadFile:@"invaders.g" IntoMemoryAt:0x0800];
    [self ReadFile:@"invaders.f" IntoMemoryAt:0x1000];
    [self ReadFile:@"invaders.e" IntoMemoryAt:0x1800];
    
    return self;
}

-(void *) frameBuffer {
    return (void*) &state->memory[0x2400];
}

-(double) timeUSec {
    struct timeval time;
    gettimeofday(&time, NULL);
    return ((double)time.tv_sec * 1E6) + ((double)time.tv_usec);
}

-(uint8_t) InSpaceInvaders:(uint8_t) port {
    unsigned char a = 0;
    switch(port) {
        case 0:
            return 1;
        case 1:
            return 0;
        case 3: {
            uint16_t v = (shift1 << 8) | shift0;
            a = ((v >> (8 - shiftOffset)) & 0xff);
        }
            break;
    }
    return a;
}

-(void) OutSpaceInvaders:(uint8_t) port value:(uint8_t)value {
    switch(port) {
        case 2:
            shiftOffset = value & 0x7;
            break;
        case 4:
            shift0 = shift1;
            shift1 = value;
            break;
    }
}

-(void) doCPU {
    double now = [self timeUSec];
    
    if(previousTimer == 0.0) {
        previousTimer = now;
        nextInterrupt = previousTimer + 16000.0;
        numInterrupt = 1;
    }
    
    if((state->intEnable) && (now > nextInterrupt)) {
        if(numInterrupt == 1) {
            generateInterrupt(state, 1);
            numInterrupt = 2;
        } else {
            generateInterrupt(state, 2);
            numInterrupt = 1;
        }
        nextInterrupt = now + 8000.0;
    }
    
    double sinceLast = now - previousTimer;
    int cyclesToCatchUp = 2 * sinceLast;
    int cycles = 0;
    
    while(cyclesToCatchUp > cycles) {
        unsigned char *op;
        op = &state->memory[state->pc];
        if(*op == 0xdb) {
            state->a = [self InSpaceInvaders:op[1]];
            state->pc += 2;
            cycles +=3;
        } else if(*op == 0xd3) {
            [self OutSpaceInvaders:op[1] value: state->a];
            state->pc += 2;
            cycles += 3;
        } else {
            cycles += emulateOp(state);
        }
        previousTimer = now;
    }
}

-(void) startEmu {
    emuTimer = [NSTimer scheduledTimerWithTimeInterval: 0.001
                                                     target:self
                                                   selector:@selector(doCPU)
                                                   userInfo:nil
                                                    repeats:YES];
}

@end
