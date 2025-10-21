#!/usr/bin/env bash

DOC_URL="$HOME/Downloads/python.pdf"
PROMPT="Summarize this document im running this on terminal so format it to look good, no markdown formatting cause im getting the response and viewing it via terminal, also format it with colors with ANSI format like this \033[97m"
GOOGLE_API_KEY="AIzaSyBZgEPTqbz8MwbuXV1LAgEguqBKiL9Y_DY"

B64FLAGS="-w0"

# Base64 encode the PDF and save the encoded version to a file
base64 $B64FLAGS "$DOC_URL" | tee encoded.txt | jq -Rs --arg prompt "$PROMPT" '
{
  contents: [{
    parts: [
      { inline_data: { mime_type: "application/pdf", data: . } },
      { text: $prompt }
    ]
  }]
}' > request.json

# Generate content using the base64 encoded PDF
curl "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=$GOOGLE_API_KEY" \
    -H 'Content-Type: application/json' \
    -X POST \
    -d @request.json \
    -o response.json \
    -s

echo
cat response.json
echo
jq -r ".candidates[].content.parts[].text" response.json | sed 's/\\033/\x1b/g'

Gemini responsesss:
{
  "candidates": [
    {
      "content": {
        "parts": [
          {
            "text": "\\033[97m\nAbsolutely! She definitely has a very charming and cute appearance.\n\n\\033[96mHer bright blue eyes,
 soft hairstyle with bangs, and the gentle expression on her face really contribute to a sweet and adorable look.\\033[97m"
          }
        ],
        "role": "model"
      },
      "finishReason": "STOP",
      "index": 0
    }
  ],
  "usageMetadata": {
    "promptTokenCount": 326,
    "candidatesTokenCount": 63,
    "totalTokenCount": 543,
    "cachedContentTokenCount": 173,
    "promptTokensDetails": [
      {
        "modality": "TEXT",
        "tokenCount": 68
      },
      {
        "modality": "IMAGE",
        "tokenCount": 258
      }
    ],
    "cacheTokensDetails": [
      {
        "modality": "IMAGE",
        "tokenCount": 137
      },
      {
        "modality": "TEXT",
        "tokenCount": 36
      }
    ],
    "thoughtsTokenCount": 154
  },
  "modelVersion": "gemini-2.5-flash",
  "responseId": "VkH2aIbVB7_l1e8P4rrl0A4"
}

#        ██████████         ██  ██████████████                
#       ██████████             ██████████████       ██████
#          ███████████████   ███   ███████████████
#            ███    ████████████████ ███    ███   
#        ███████████████   ███   ███████████      
#   ██████████ ███     ███████   ████ ███    ███       
#  █████████████████████████ ██████████████████        
#  
#     ▄████████ ███    █▄   ▄████████  ▄████████    ▄████████    ▄████████    ▄████████ 
#    ███    ███ ███    ███ ███    ███ ███    ███   ███    ███   ███    ███   ███    ███ 
#    ███    █▀  ███    ███ ███    █▀  ███    █▀    ███    █▀    ███    █▀    ███    █▀  
#    ███        ███    ███ ███        ███         ▄███▄▄▄       ███          ███        
#  ▀███████████ ███    ███ ███        ███        ▀▀███▀▀▀     ▀███████████ ▀███████████ 
#           ███ ███    ███ ███    █▄  ███    █▄    ███    █▄           ███          ███ 
#     ▄█    ███ ███    ███ ███    ███ ███    ███   ███    ███    ▄█    ███    ▄█    ███ 
#   ▄████████▀  ████████▀  ████████▀  ████████▀    ██████████  ▄████████▀   ▄████████▀  
#                                                                                       
#        ::::::::  :::    :::  ::::::::   ::::::::  :::::::::: ::::::::   :::::::: 
#      :+:    :+: :+:    :+: :+:    :+: :+:    :+: :+:       :+:    :+: :+:    :+: 
#     +:+        +:+    +:+ +:+        +:+        +:+       +:+        +:+         
#    +#++:++#++ +#+    +:+ +#+        +#+        +#++:++#  +#++:++#++ +#++:++#++   
#          +#+ +#+    +#+ +#+        +#+        +#+              +#+        +#+    
#  #+#    #+# #+#    #+# #+#    #+# #+#    #+# #+#       #+#    #+# #+#    #+#     
#  ########   ########   ########   ########  ########## ########   ########       
