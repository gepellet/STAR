#include "IncludeDefine.h"
#include "Parameters.h"
#include "Transcript.h"
#include "ReadAlign.h"
//#include "SequenceFuns.h"
//#include "stitchWindowAligns.h"
//#include "sjSplitAlign.cpp"
//#include "PackedArray.h"
//#include "alignSmithWaterman.h"
//#include "GlobalVariables.h"
//#include <time.h>

void ReadAlign::chimericDetection() {

    //output chains for out-of-STAR chimeric detection
    #ifdef OUTPUT_localChains
    {
        P->inOut->outLocalChains << readName <<"\t"<< Read0[0] <<"\t"<< Read0[1] << "\n";
        for (uint iw=0; iw<nW; iw++) {
            for (uint itr=0;itr<nWinTr[iw];itr++) {
                P->inOut->outLocalChains << trAll[iw][itr]->maxScore<<"\t"<< trAll[iw][itr]->Chr<<"\t"<<trAll[iw][itr]->Str<<"\t"<<trAll[iw][itr]->nExons;
                for (uint ib=0;ib<trAll[iw][itr]->nExons;ib++) {                    
                    P->inOut->outLocalChains <<"\t"<< trAll[iw][itr]->exons[ib][EX_G]-P->chrStart[trAll[iw][itr]->Chr] \
                                             <<"\t"<< trAll[iw][itr]->exons[ib][EX_R] <<"\t"<< trAll[iw][itr]->exons[ib][EX_L];
                };
                P->inOut->outLocalChains <<"\n";
            };
        };
    };
    #endif
    //////////////////// chimeras
    //stich windows => chimeras
    //stich only the best window with one of the lower score ones for now - do not stich 2 lower score windows
    //stitch only one window on each end of the read

    if (P->chimSegmentMin>0 && nW>1 && trBest->rLength >= P->chimSegmentMin \
            && ( trBest->exons[trBest->nExons-1][EX_R] + trBest->exons[trBest->nExons-1][EX_L] + P->chimSegmentMin <= Lread \
              || trBest->exons[0][EX_R] >= P->chimSegmentMin ) \
             && trBest->nextTrScore+P->outFilterMultimapScoreRange < trBest->maxScore \
             && trBest->intronMotifs[0]==0 && (trBest->intronMotifs[1]==0 || trBest->intronMotifs[2]==0) ) { 
            //there is unmapped space at the start/end, and the main window is not a multimapping window, and non non-canonical junctions, and consistend junction motif
        int chimScoreBest=0,chimScoreNext=0;
        trChim[0]=*trBest;

        uint roStart1=trBest->Str==0 ? trBest->exons[0][EX_R] : Lread - trBest->exons[trBest->nExons-1][EX_R] - trBest->exons[trBest->nExons-1][EX_L];
        uint roEnd1=trBest->Str==0 ? trBest->exons[trBest->nExons-1][EX_R] + trBest->exons[trBest->nExons-1][EX_L] - 1 : Lread - trBest->exons[0][EX_R] - 1;
        if (roStart1>readLength[0]) roStart1--;
        if (roEnd1>readLength[0]) roEnd1--;

        uint chimStrBest=0;
        if (trBest->intronMotifs[1]==0 && trBest->intronMotifs[2]==0) {//strand is undefined
            chimStr=0;
        } else if ( (trBest->Str==0) == (trBest->intronMotifs[1]>0)) {//strand the same as RNA
            chimStr=1;
        } else {//strand opposite to RNA
            chimStr=2;
        };

        for (uint iW=0; iW<nW; iW++) {//check all other windows for chimeras
            for (uint iWt=0; iWt<nWinTr[iW]; iWt++){//cycl over transcripts in the window    
                if (trBest!=trAll[iW][0] && iWt>0) break; //for all windows except that of the best transcript - hceck only iWt=0 (best trnascripts)
                if (trBest==trAll[iW][0] && iWt==0) continue;
    //                 {//same window
    //                     if (iWt==0) continue; //do not check the best transcript itself
    //                     if (trBest->exons[0][EX_R]<=trAll[iW][iWt]->exons[0][EX_R]) {
    //                         //start of the last Best exon is before end of the first Chim exon
    //                         if (trBest->exons[trBest->nExons-1][EX_G]<trAll[iW][iWt]->exons[0][EX_G]+trAll[iW][iWt]->exons[0][EX_L]) continue;
    //                     } else {
    //                         if (trAll[iW][iWt]->exons[trAll[iW][iWt]->nExons-1][EX_G]<trBest->exons[0][EX_G]+trBest->exons[0][EX_L]) continue;                        
    //                     };
    //                 };

                if (trAll[iW][iWt]->intronMotifs[0]>0) continue; //do not stitch a window to itself, or to a window with non-canonical junctions
                uint chimStr1;
                if (trAll[iW][iWt]->intronMotifs[1]==0 && trAll[iW][iWt]->intronMotifs[2]==0) {//strand is undefined
                    chimStr1=0;
                } else if ( (trAll[iW][iWt]->Str==0) == (trAll[iW][iWt]->intronMotifs[1]>0)) {//strand the same as RNA
                    chimStr1=1;
                } else {//strand opposite to RNA
                    chimStr1=2;
                };            

                if (chimStr!=0 && chimStr1!=0 && chimStr!=chimStr1) continue; //chimeric segments have to have consitent strands

                uint roStart2=trAll[iW][iWt]->Str==0 ? trAll[iW][iWt]->exons[0][EX_R] : Lread - trAll[iW][iWt]->exons[trAll[iW][iWt]->nExons-1][EX_R] - trAll[iW][iWt]->exons[trAll[iW][iWt]->nExons-1][EX_L];
                uint roEnd2=trAll[iW][iWt]->Str==0 ? trAll[iW][iWt]->exons[trAll[iW][iWt]->nExons-1][EX_R] + trAll[iW][iWt]->exons[trAll[iW][iWt]->nExons-1][EX_L] - 1 : Lread - trAll[iW][iWt]->exons[0][EX_R] - 1;
                if (roStart2>readLength[0]) roStart2--;
                if (roEnd2>readLength[0]) roEnd2--;          

                uint chimOverlap = roStart2>roStart1 ?  (roStart2>roEnd1 ? 0 : roEnd1-roStart2+1) : (roEnd2<roStart1 ? 0 : roEnd2-roStart1+1);
                bool diffMates=(roEnd1 < readLength[0] && roStart2 >= readLength[0]) || (roEnd2 < readLength[0] && roStart1 >= readLength[0]);

                //segment lengths && (different mates || small gap between segments)
                if (roEnd1 > P->chimSegmentMin + roStart1 + chimOverlap && roEnd2> P->chimSegmentMin + roStart2 + chimOverlap  \
                    && ( diffMates || ( (roEnd1 + P->maxChimReadGap + 1) >= roStart2 && (roEnd2 + P->maxChimReadGap + 1) >= roStart1 ) ) ) {
                                           //maxChimReadGap=0 in Parameters.cpp

                    int chimScore=trBest->maxScore + trAll[iW][iWt]->maxScore - (int)chimOverlap; //subtract overlap to avoid double counting

                    if (chimScore > chimScoreBest && chimScore >= P->chimScoreMin && chimScore+P->chimScoreDropMax >= (int) (readLength[0]+readLength[1]) ) {
                        trChim[1]=*trAll[iW][iWt];                                      
                        chimScoreNext=chimScoreBest;
                        chimScoreBest=chimScore;
                        trChim[1].roStart = trChim[1].roStr ==0 ? trChim[1].rStart : Lread - trChim[1].rStart - trChim[1].rLength;
                        trChim[1].cStart  = trChim[1].gStart - P->chrStart[trChim[1].Chr];      
                        chimStrBest=chimStr1;
                    } else if (chimScore>chimScoreNext) {//replace the nextscore if it's not the best one and is higher than the previous one
                        chimScoreNext=chimScore;              
                    };
                };
            };//cycle over window transcripts
        };//cyecl over windows

        if (chimStr==0) chimStr=chimStrBest;

        chimN=0;
        if (chimScoreNext + P->chimScoreSeparation < chimScoreBest) {//report only if chimera is unique

            if (trChim[0].roStart > trChim[1].roStart) swap (trChim[0],trChim[1]);

            uint e0 = trChim[0].Str==1 ? 0 : trChim[0].nExons-1;
            uint e1 = trChim[1].Str==0 ? 0 : trChim[1].nExons-1;

            uint chimRepeat0=0,chimRepeat1=0,chimJ0=0,chimJ1=0;
            int chimMotif=0;
            chimN=2;            
            if ( trChim[0].exons[e0][EX_iFrag] != trChim[1].exons[e1][EX_iFrag] ) {//mates bracket the chimeric junction
                chimN=2;
                chimRepeat=0;
                chimMotif=-1;
                if (trChim[0].Str==1) {                    
                    chimJ0=trChim[0].exons[e0][EX_G]-1;
                } else {
                    chimJ0=trChim[0].exons[e0][EX_G]+trChim[0].exons[e0][EX_L];                            
                };            
                if (trChim[1].Str==1) {                    
                    chimJ1=trChim[1].exons[e1][EX_G]-1;
                } else {
                    chimJ1=trChim[1].exons[e1][EX_G]+trChim[1].exons[e1][EX_L];                            
                };                    
            } else {//check and shift chimeric junction if necessary
                if (trChim[0].exons[e0][EX_L]>=P->chimJunctionOverhangMin && trChim[1].exons[e1][EX_L]>=P->chimJunctionOverhangMin ) {//large enough overhanh required
                    uint roStart0 = trChim[0].Str==0 ? trChim[0].exons[e0][EX_R] : Lread - trChim[0].exons[e0][EX_R] - trChim[0].exons[e0][EX_L];
                    uint roStart1 = trChim[1].Str==0 ? trChim[1].exons[e1][EX_R] : Lread - trChim[1].exons[e1][EX_R] - trChim[1].exons[e1][EX_L];

                    uint jR, jRbest=0;
                    int jScore=0,jMotif=0,jScoreBest=-999999,jScoreJ=0;
                    for (jR=0; jR<roStart1+trChim[1].exons[e1][EX_L]-roStart0-1; jR++) {//scan through the exons to find a canonical junction, and check for mismatches

                        if (jR==readLength[0]) jR++; //skip the inter-mate base

                        char bR=Read1[0][roStart0+jR];

                        char b0,b1;
                        if (trChim[0].Str==0) {
                            b0=G[trChim[0].exons[e0][EX_G]+jR];
                        } else {
                            b0=G[trChim[0].exons[e0][EX_G]+trChim[0].exons[e0][EX_L]-1-jR];
                            if (b0<4) b0=3-b0;
                        };

                        if (trChim[1].Str==0) {
                            b1=G[trChim[1].exons[e1][EX_G]-roStart1+roStart0+jR];
                        } else {
                            b1=G[trChim[1].exons[e1][EX_G]+trChim[1].exons[e1][EX_L]-1+roStart1-roStart0-jR];
                            if (b1<4) b1=3-b1;
                        };

                        if (b0>3 || b1>3 || bR>3) {//chimera is not called if there are Ns in the genome or in the read
                            chimN=0;
                            break;
                        };

                        char b01,b02,b11,b12;
                        if (trChim[0].Str==0) {
                            b01=G[trChim[0].exons[e0][EX_G]+jR+1];
                            b02=G[trChim[0].exons[e0][EX_G]+jR+2];                                
                        } else {
                            b01=G[trChim[0].exons[e0][EX_G]+trChim[0].exons[e0][EX_L]-1-jR-1];
                            if (b01<4) b01=3-b01;
                            b02=G[trChim[0].exons[e0][EX_G]+trChim[0].exons[e0][EX_L]-1-jR-2];
                            if (b02<4) b02=3-b02;                                
                        };      
                        if (trChim[1].Str==0) {
                            b11=G[trChim[1].exons[e1][EX_G]-roStart1+roStart0+jR-1];
                            b12=G[trChim[1].exons[e1][EX_G]-roStart1+roStart0+jR];                                
                        } else {
                            b11=G[trChim[1].exons[e1][EX_G]+trChim[1].exons[e1][EX_L]-1+roStart1-roStart0-jR+1];
                            if (b11<4) b11=3-b11;
                            b12=G[trChim[1].exons[e1][EX_G]+trChim[1].exons[e1][EX_L]-1+roStart1-roStart0-jR];
                            if (b12<4) b12=3-b12;                                
                        };        

                        jMotif=0;                        
                        if (b01==2 && b02==3 && b11==0 && b12==2) {//GTAG
                            if (chimStr!=2) {
                                jMotif=1;
                            };
                        } else if(b01==1 && b02==3 && b11==0 && b12==1) {//CTAC
                            if (chimStr!=1) {
                                jMotif=2;
                            };            
                        };  

                        if (bR==b0 && bR!=b1) {
                            jScore++;
                        } else if (bR!=b0 && bR==b1) {
                            jScore--;
                        };

                        jScoreJ =jMotif==0 ? jScore +  P->chimScoreJunctionNonGTAG : jScore ;

                        if ( jScoreJ > jScoreBest || (jScoreJ == jScoreBest && jMotif>0) ) {
                            chimMotif=jMotif;
                            jRbest=jR;
                            jScoreBest=jScoreJ;
                        };
                    };//jR cycle
                    if (chimN>0) {//else the chimera was rejected because of mismatches

                        //shift junction in trChim
                        if (trChim[0].Str==1) {
                            trChim[0].exons[e0][EX_R] +=trChim[0].exons[e0][EX_L]-jRbest-1;
                            trChim[0].exons[e0][EX_G] +=trChim[0].exons[e0][EX_L]-jRbest-1;
                            trChim[0].exons[e0][EX_L]=jRbest+1;                        
                            chimJ0=trChim[0].exons[e0][EX_G]-1;
                        } else {
                            trChim[0].exons[e0][EX_L]=jRbest+1;
                            chimJ0=trChim[0].exons[e0][EX_G]+trChim[0].exons[e0][EX_L];                            
                        };                            

                        if (trChim[1].Str==0) {
                            trChim[1].exons[e1][EX_R] +=roStart0+jRbest+1-roStart1;
                            trChim[1].exons[e1][EX_G] +=roStart0+jRbest+1-roStart1;
                            trChim[1].exons[e1][EX_L]=roStart1+trChim[1].exons[e1][EX_L]-roStart0-jRbest-1;  
                            chimJ1=trChim[1].exons[e1][EX_G]-1;
                        } else {
                            trChim[1].exons[e1][EX_L]=roStart1+trChim[1].exons[e1][EX_L]-roStart0-jRbest-1;  
                            chimJ1=trChim[1].exons[e1][EX_G]+trChim[1].exons[e1][EX_L];  
                        };
                        //find repeats
                        char b0,b1;
                        uint jR;
                        for (jR=0;jR<100;jR++) {//forward check
                            if (trChim[0].Str==0) {
                                b0=G[chimJ0+jR];
                            } else {
                                b0=G[chimJ0-jR];
                                if (b0<4) b0=3-b0;
                            };

                            if (trChim[1].Str==0) {
                                b1=G[chimJ1+1+jR];
                            } else {
                                b1=G[chimJ1-1-jR];
                                if (b1<4) b1=3-b1;
                            };
                            if (b0!=b1) break;
                        };
                        chimRepeat1=jR;
                        for (jR=0;jR<100;jR++) {//reverse check
                            if (trChim[0].Str==0) {
                                b0=G[chimJ0-1-jR];
                            } else {
                                b0=G[chimJ0+1+jR];
                                if (b0<4) b0=3-b0;
                            };

                            if (trChim[1].Str==0) {
                                b1=G[chimJ1-jR];
                            } else {
                                b1=G[chimJ1+jR];
                                if (b1<4) b1=3-b1;
                            };
                            if (b0!=b1) break;
                        };                        
                        chimRepeat0=jR;
                    };//chimN>0
                };//large enough overhang
            };//chimeric junction is within a mate

            //debug
    //             cout << readName <<"\t"<< (trChim[0].Str==0 ? chimJ1-chimJ0 : chimJ0-chimJ1) << "\t"<< (chimMotif>=0 ? P->alignIntronMax :  P->alignMatesGapMax)<<"\n";
    //             cout <<  chimRepeat0 <<"\t"<<trChim[0].exons[e0][EX_L]<<"\n";
            //chimeric alignments output
            if ( chimN==2 && trChim[0].exons[e0][EX_L]>=P->chimJunctionOverhangMin+chimRepeat0 \
                    && trChim[1].exons[e1][EX_L]>=P->chimJunctionOverhangMin+chimRepeat1 \
                    && ( trChim[0].Str!=trChim[1].Str ||  trChim[0].Chr!=trChim[1].Chr \
                    || (trChim[0].Str==0 ? chimJ1-chimJ0+1LLU : chimJ0-chimJ1+1LLU) > (chimMotif>=0 ? P->alignIntronMax :  P->alignMatesGapMax) ) )
            {//unique chimeras only && minOverhang1 
             //&& minOverhang2
             //&& (diff str || diff chr || 
             //|| gap > (alignIntronMax,alignMatesGapMax) ) negative gap = very large # because of uint

                if (trChim[0].exons[0][EX_iFrag]!=trChim[0].exons[trChim[0].nExons-1][EX_iFrag]) {
                    trChim[0].primaryFlag=true;//paired portion is primary
                    trChim[1].primaryFlag=false;
                } else if (trChim[1].exons[0][EX_iFrag]!=trChim[1].exons[trChim[1].nExons-1][EX_iFrag]) {
                    trChim[1].primaryFlag=true;//paired portion is primary
                    trChim[0].primaryFlag=false;                    
                } else if (trChim[0].exons[0][EX_iFrag]!=trChim[1].exons[0][EX_iFrag]) {
                    trChim[1].primaryFlag=true;//paired portion is primary
                    trChim[0].primaryFlag=true;
                } else  {//two chimeric segments are on the same mate, another mate not mapped
                    trChim[0].primaryFlag=true;//paired portion is primary
                    trChim[1].primaryFlag=false;
                };

                for (uint iTr=0;iTr<chimN;iTr++) {//write all chimeric pieces
                    if (P->readNmates==2) {
                        outputTranscriptSAM(trChim[iTr], chimN, iTr, trChim[1-iTr].Chr, trChim[1-iTr].exons[0][EX_G], (int) (trChim[1-iTr].Str!=trChim[1-iTr].exons[0][EX_iFrag]), -1, NULL, &chunkOutChimSAM);
                    } else {
                        outputTranscriptSAM(trChim[iTr], chimN, iTr, -1, -1, -1, -1, NULL, &chunkOutChimSAM);
                    };                        
                };        
                //junction + SAMp
                chunkOutChimJunction << P->chrName[trChim[0].Chr] <<"\t"<< chimJ0 - P->chrStart[trChim[0].Chr]+1 <<"\t"<< (trChim[0].Str==0 ? "+":"-") \
                        <<"\t"<< P->chrName[trChim[1].Chr] <<"\t"<< chimJ1 - P->chrStart[trChim[1].Chr]+1 <<"\t"<< (trChim[1].Str==0 ? "+":"-") \
                        <<"\t"<< chimMotif <<"\t"<< chimRepeat0  <<"\t"<< chimRepeat1 <<"\t"<< readName+1 \
                        <<"\t"<< trChim[0].exons[0][EX_G] - P->chrStart[trChim[0].Chr]+1 <<"\t"<< outputTranscriptCIGARp(trChim[0]) \
                        <<"\t"<< trChim[1].exons[0][EX_G] - P->chrStart[trChim[1].Chr]+1 <<"\t"<<  outputTranscriptCIGARp(trChim[1]) <<"\n"; //<<"\t"<< trChim[0].exons[0][EX_iFrag]+1 --- no need for that, since trChim[0] is always on the first mate
            };
        };//chimeric score
    };//chimeric search
};//END