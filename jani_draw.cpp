//static void
//SubmitQuad(jani_rect Rect)
//{
//    u32 BaseIndex = FrameDrawList.ElementsCount;
//
//    u32 Indices[6] = { BaseIndex + 0, BaseIndex + 1, BaseIndex + 2,
//                       BaseIndex + 3, BaseIndex + 2, BaseIndex + 1 };
//
//    for(u32 I = 0; I < 6; I++)
//    {
//        FrameDrawList.Indices.Push(Indices[I]);
//    }
//
//    jani_draw_input TopLeft = {
//        .Color = {1.0f, 1.0f, 1.0f, 1.0f},
//        .Pos = {Rect.Pos.x, Rect.Pos.y},
//        .Tex = {0.0f, 0.0f},
//     };

//    jani_draw_input TopRight = {
//        .Color = {1.0f, 1.0f, 1.0f, 1.0f},
//        .Pos = {Rect.Pos.x + Rect.Size.x, Rect.Pos.y},
//        .Tex = {0.0f, 0.0f},
//j     };

//    jani_draw_input BottomRight = {
//        .Color = {1.0f, 1.0f, 1.0f, 1.0f},
//        .Pos = {Rect.Pos.x + Rect.Size.x, Rect.Pos.y + Rect.Size.y},
//        .Tex = {0.0f, 0.0f},
//     };

//    jani_draw_input BottomLeft = {
//        .Color = {1.0f, 1.0f, 1.0f, 1.0f},
//        .Pos = {Rect.Pos.x, Rect.Pos.y + Rect.Size.y},
//        .Tex = {0.0f, 0.0f},
//     };

//    PushAndCopy(sizeof(jani_draw_input), &TopLeft    , &FrameDrawList.ElementsBuffer);
//    PushAndCopy(sizeof(jani_draw_input), &TopRight   , &FrameDrawList.ElementsBuffer);
//    PushAndCopy(sizeof(jani_draw_input), &BottomLeft , &FrameDrawList.ElementsBuffer);
//    PushAndCopy(sizeof(jani_draw_input), &BottomRight, &FrameDrawList.ElementsBuffer);


//    draw_command& ActiveCommand  = FrameDrawList.GetDrawCommand();
//    ActiveCommand.ElementsCount += 6;
//}
