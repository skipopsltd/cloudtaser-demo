{{- define "cloudcondom.labels" -}}
app.kubernetes.io/version: {{ .Chart.AppVersion | quote }}
app.kubernetes.io/managed-by: {{ .Release.Service }}
app.kubernetes.io/part-of: cloudcondom
helm.sh/chart: {{ .Chart.Name }}-{{ .Chart.Version }}
{{- end }}

{{- define "cloudcondom.namespace" -}}
{{ .Values.namespace | default "cloudcondom-system" }}
{{- end }}

{{- define "cloudcondom.operator.image" -}}
{{ .Values.operator.image.repository }}:{{ .Values.operator.image.tag }}
{{- end }}

{{- define "cloudcondom.wrapper.image" -}}
{{ .Values.wrapper.image.repository }}:{{ .Values.wrapper.image.tag }}
{{- end }}

{{- define "cloudcondom.ebpf.image" -}}
{{ .Values.ebpf.image.repository }}:{{ .Values.ebpf.image.tag }}
{{- end }}
