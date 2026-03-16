{{- define "cloudtaser.labels" -}}
app.kubernetes.io/version: {{ .Chart.AppVersion | quote }}
app.kubernetes.io/managed-by: {{ .Release.Service }}
app.kubernetes.io/part-of: cloudtaser
helm.sh/chart: {{ .Chart.Name }}-{{ .Chart.Version }}
{{- end }}

{{- define "cloudtaser.namespace" -}}
{{ .Values.namespace | default "cloudtaser-system" }}
{{- end }}

{{- define "cloudtaser.operator.image" -}}
{{ .Values.operator.image.repository }}:{{ .Values.operator.image.tag }}
{{- end }}

{{- define "cloudtaser.wrapper.image" -}}
{{ .Values.wrapper.image.repository }}:{{ .Values.wrapper.image.tag }}
{{- end }}

{{- define "cloudtaser.ebpf.image" -}}
{{ .Values.ebpf.image.repository }}:{{ .Values.ebpf.image.tag }}
{{- end }}
