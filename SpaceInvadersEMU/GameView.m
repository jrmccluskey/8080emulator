//
//  GameView.m
//  SpaceInvadersEMU
//
//  Created by mcjack on 8/17/18.
//  Copyright © 2018 Jack McCluskey. All rights reserved.
//

#import "GameView.h"

@implementation GameView

- (void)drawRect:(NSRect)dirtyRect {
    CGContextRef context = [[NSGraphicsContext currentContext] graphicsPort];
    CGContextSetRGBFillColor (context, 0.0, 0.2, 0.5, 1.0);
    CGContextFillRect(context, self.bounds);
}

@end
