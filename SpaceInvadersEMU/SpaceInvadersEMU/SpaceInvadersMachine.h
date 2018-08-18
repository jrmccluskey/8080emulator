//
//  SpaceInvadersMachine.h
//  SpaceInvadersEMU
//
//  Created by mcjack on 8/17/18.
//  Copyright © 2018 Jack McCluskey. All rights reserved.
//

#import <Foundation/Foundation.h>
#include"emulatorShell.c"

@interface SpaceInvadersMachine : NSObject {
    state8080 *state;
    
    double previousTimer;
    double nextInterrupt;
    double numInterrupt;
    
    NSTimer *emuTimer;
    uint8_t shift0;
    uint8_t shift1;
    uint8_t shiftOffset;
}

-(double) timeUSec;

-(void) ReadFile:(NSString*)filename IntoMemoryAt:(uint32_t)memOffset;
-(id) init;
-(void) doCPU;
-(void) startEmu;

-(void *) frameBuffer;

@end
